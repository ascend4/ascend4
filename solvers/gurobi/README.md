# Gurobi LP/MIP solver

Optional ASCEND solver registered as `Gurobi`. Solves LPs and linear MIPs:
affine objectives (minimize/maximize), equality/inequality rows, variable bounds,
and `solver_int`, `solver_binary` and `solver_semi` variables. It does not yet
export quadratic/nonlinear models, indicators, SOS groups or general constraints.

## Build and license

Install the Gurobi C SDK with headers in `~/.local/include` and its shared library
in `~/.local/lib`. For the Gurobi 13.0.3 Linux x86-64 tarball in your home
directory, extract just the C header, runtime library and linker symlink:

```sh
mkdir -p "$HOME/.local"
tar -xzf "$HOME/gurobi13.0.3_linux64.tar.gz" \
    -C "$HOME/.local" --strip-components=2 --no-same-owner \
    gurobi1303/linux64/include/gurobi_c.h \
    gurobi1303/linux64/lib/libgurobi.so.13.0.3 \
    gurobi1303/linux64/lib/libgurobi130.so
```

Adjust the archive and member names for other releases. Both library entries
are needed: `libgurobi130.so` links to the versioned runtime. This command
replaces any existing files at those three paths; it does not install a license
or the Gurobi command-line tools. Then build with:

```sh
scons -j4 WITH_GUROBI=1
```

`GUROBI_PREFIX=/path/to/prefix` overrides the SDK location. SCons derives the
library name from `gurobi_c.h`; `GUROBI_LIB` can override it. SDK detection only
links a version query and does not require a license. Gurobi remains opt-in.

The `a4` launcher automatically uses `~/.config/ascend/gurobi.lic` when that file
exists and `GRB_LICENSE_FILE` is not already set. Dropping the file there is
sufficient; no shell configuration is needed. An explicit environment variable
(even an empty one) is preserved. Without the ASCEND-specific file, `a4` leaves
Gurobi's native license search untouched.

For a license shared across Gurobi applications, the vendor's standard per-user
location is `~/gurobi.lic`; on Linux `/opt/gurobi/gurobi.lic` is also a standard
location. A license alongside `~/.local/lib/libgurobi130.so` is not a documented
default. See [Gurobi's license-location guidance](https://support.gurobi.com/hc/en-us/articles/360013417211-Where-do-I-place-the-Gurobi-license-file-gurobi-lic).
No existing license is moved or copied by ASCEND. Direct launches that bypass
`a4` need a native location or an explicit `GRB_LICENSE_FILE`.

Keep the license file private (mode 0600). No license content belongs in the
repository or SCons configuration, and no `secrets.ini` entry is needed. The
adapter starts a licensed environment on solving or an explicit solver-details
query, not on registration, version queries, eligibility checks or ASCEND
presolve. Network licenses need network access.
Native logging is disabled before environment startup to avoid exposing license
details; API failures report the Gurobi error number.

`./a4 solvers` reports, for example, `Gurobi: 13.0.3 (licensed; LICENSEID=123456)`
when Gurobi can initialise a licensed environment. The numeric ID is queried
from that environment, not extracted from the license file, so you can identify
the validated WLS license in the [Gurobi Web License Manager](https://license.gurobi.com/).
The [LicenseID parameter](https://docs.gurobi.com/projects/optimizer/en/13.0/reference/parameters.html#parameterlicenseid)
is intended for WLS licenses; if it is unset or cannot be queried, the listing
says `licensed; LICENSEID unavailable` without treating the valid license as
invalid. Access IDs, secrets and tokens remain hidden.

It reports `license unavailable (Gurobi error 10009)`
for missing, invalid or otherwise unavailable licensing, or `license validation
inconclusive` with the error code for other initialization failures. These are
live checks, not cached file-existence checks; they can contact a license server.
The temporary environment is freed immediately, without solving a model. A
successful check does not imply an unrestricted license or guarantee a later
solve of an arbitrary size. Raw license contents and native error messages are never
included in the listing.

Select `Gurobi` in the GUI or with `SOLVER Gurobi;` in an ASCEND METHOD.
The small executable example also checks its solution:

```sh
./a4 run models/test/gurobi/gurobi_demo.a4c
```

For a compact, dimensional example, start with the
[alloy-blending introduction](../../models/alloy_blending.md). The longer-form
showcase is retained separately as `models/alloy_blending_detailed.a4c`:

```sh
./a4 run models/alloy_blending.a4c
```

The [multiperiod steel-production example](../../models/steel_production.md)
adds inventory balances linking four weeks, with a profit-maximising objective:

```sh
./a4 run models/steel_production.a4c
```

The third showcase, [refinery planning](../../models/refinery.md), ports GAMS
MARCO with SI data, crude-specific process yields and dimensional blend-quality
constraints. It includes the original and tighter fuel-oil sulfur limits:

```sh
./a4 run models/refinery.a4c
./a4 run models/refinery.a4c --model refinery_low_sulfur
```

For a dimensioned MIP showcase, [Food Manufacture II](../../models/food_manufacture_2.md)
adds binary ingredient choices to six months of oil purchasing, refining and
blending. It uses ordinary linear constraints and works with either solver:

```sh
./a4 run models/food_manufacture_2.a4c
./a4 run models/food_manufacture_2.a4c --model food_manufacture_2_highs
```

## Options and semantics

- `nonlin` (default false): explicitly request **one tangent LP/MIP** at the current
  model point, retaining unrelaxed discrete domains. This is not an SQP loop or a
  solution of the original nonlinear problem. External
  relations require this option and working derivatives. Exact LP/MIP mode rejects
  black boxes and any expression whose affine form cannot be established.
- `relaxed` (default false): explicitly request the continuous relaxation of
  discrete solver variables. Individual `relaxed` variable flags are respected.
  Binary relaxations retain [0,1] bounds. Semicontinuous variables permit zero
  or a value in [L,U]; for 0 < L <= U the relaxation is [0,U], not [L,U].
- `varnom_scale`, `relnom_scale` (default false): use the shared nominal scaling.
  Unrelaxed integer/binary/semicontinuous columns are not nominal-scaled.
- `method`: -1 automatic, 0 primal simplex, 1 dual simplex, 2 barrier,
  3 concurrent, 4 deterministic concurrent. Controls LP solves and the MIP
  root relaxation; `node_method` controls subsequent MIP node relaxations.
- `presolve`: -1 automatic, 0 off, 1 conservative, 2 aggressive.
- `dual_reductions` (default true): disable and re-solve to distinguish
  infeasible from unbounded after an ambiguous `INF_OR_UNBD` result.
- `time_limit` (seconds), `threads` (0 automatic), `feasibility_tol`,
  `optimality_tol`, and `progress_callbacks`.
- `mip_rel_gap` (default 1e-4), `mip_abs_gap` (default 1e-6): optimality-gap
  targets, with the same names/defaults as HiGHS. Absolute gaps use the
  objective's ASCEND internal units (e.g. USD/s for monetary flow), not its
  display units. "Optimal" means optimal within the configured tolerances.
- `integrality_tol` (default 1e-5): Gurobi's integer/semi feasibility tolerance.
- `node_limit` (default unlimited), `solution_limit` (default 2000000000),
  `random_seed` (default 0).

The additional tuning subset follows [Gurobi's parameter guidelines](https://docs.gurobi.com/projects/optimizer/en/current/concepts/parameters/guidelines.html).
These options are available through the usual METHOD `OPTION`, Python and GUI
parameter interfaces. Defaults below match Gurobi 13.0; automatic/default
settings are a sensible starting point.

| ASCEND option | Native parameter | Default | Supported values / purpose |
|---|---|---:|---|
| `mip_focus` | MIPFocus | 0 | 0 balanced; 1 feasible solutions; 2 optimality proof; 3 best bound |
| `heuristics` | Heuristics | 0.05 | 0–1: heuristic effort fraction |
| `cuts` | Cuts | -1 | -1 auto; 0 off; 1–3 increasing aggressiveness |
| `symmetry` | Symmetry | -1 | -1 auto; 0 off; 1 conservative; 2 aggressive |
| `integrality_focus` | IntegralityFocus | false | Reduce effects of near-integer solutions, e.g. with big-M constraints |
| `numeric_focus` | NumericFocus | 0 | 0 auto; 1–3 increasing numerical care |
| `scale_flag` | ScaleFlag | -1 | -1 auto; 0 off; 1–3 scaling strategies (2 geometric mean) |
| `aggregate` | Aggregate | 1 | Presolve aggregation: 0 off; 1 moderate; 2 aggressive |
| `work_limit` | WorkLimit | 1e100 | Nonnegative work-unit budget; 1e100 means unlimited |
| `soft_mem_limit` | SoftMemLimit | 1e100 | Nonnegative solver memory budget in decimal GB; 1e100 means unlimited |
| `iteration_limit` | IterationLimit | 1e100 | Nonnegative simplex iteration budget, including MIP nodes |
| `bar_iter_limit` | BarIterLimit | 1000 | 0–2000000000 barrier iterations |
| `bar_conv_tol` | BarConvTol | 1e-8 | 0–1 barrier convergence tolerance |
| `crossover` | Crossover | -1 | -1 auto; 0 disabled; 1–4 push/cleanup strategies |
| `node_method` | NodeMethod | -1 | -1 auto; 0 primal simplex; 1 dual simplex; 2 barrier |

For example, in an existing model's solver-selection METHOD:

```ascend
SOLVER Gurobi;
OPTION mip_focus 1;
OPTION heuristics 0.2;
OPTION work_limit 100.0;
OPTION soft_mem_limit 2.0;
```

`OPTION` is typed: use real literals such as `100.0` for real-valued limits,
integer literals for integer options, and `TRUE`/`FALSE` for Boolean options.

`work_limit` is not seconds: deterministic stopping requires identical hardware,
model and settings. `soft_mem_limit` is a graceful, approximate limit on Gurobi's
environment memory, not on ASCEND's total process memory. It may be exceeded
between checks. `scale_flag` controls native scaling independently of ASCEND's
`varnom_scale` and `relnom_scale`; all reported results use the exported model's
units with native scaling undone.

`crossover=0` returns an interior barrier solution, without a simplex basis.
For a MIP root, disabling crossover requires both `method=2` and `node_method=2`.
Barrier convergence tolerance is distinct from the adapter's primal-feasibility
acceptance test. Gurobi may finish by crossover after reaching a barrier iteration
limit; the native final status determines whether the overall solve converged.

Ranges and finite numeric values are checked before acquiring a license; native
parameter setters are checked and their values read back. Parameters are reapplied
on each solve/resolve. A parameter error identifies its ASCEND/native name without
printing raw native messages or license details.

This is intentionally not a general Gurobi parameter-file passthrough. Individual
cut families, solution-pool search, MIP starts/hints, tuning, distributed solving,
license credentials, native log files and unsupported nonlinear/general-constraint
controls are not exposed. In particular, native logging remains suppressed before
license startup. PDHG and deprecated method variants are outside this subset.

ASCEND presolve assembles the LP/MIP and sets its status kind without acquiring
a license; Gurobi's own presolve runs during optimization.
No source model bounds or relaxation flags are modified. Exported bounds honour
the declared variable domain. A feasible primal solution is
written back with scaling undone; fixed values are preserved and residuals are
refreshed. A time/work/node/solution/memory limit or interruption can return a feasible MIP
incumbent without marking the solve converged. Without a feasible solution,
variables are not overwritten. Integrality is checked as well as linear
constraint and bound feasibility; values are not blindly rounded.
The LP status includes the objective constant, primal feasibility and iteration
counts. `INF_OR_UNBD` is retained as ambiguous, not labelled proven infeasible.
MIP status additionally includes incumbent/best bound, relative/absolute gap,
node count and stored solution count. Unavailable bounds/gaps are not reported
as finite values. The native model status retains the precise termination reason;
node, solution and work limits also set ASCEND's generic iteration-limit flag.
Soft-memory termination is reported as `MEM_LIMIT`, not infeasibility or an
interruption; ASCEND has no dedicated memory-limit flag.
Interrupts are checked even when progress reporting is disabled.

`resolve` rebuilds coefficients, bounds and offsets before solving, so changes
to fixed parameters and relaxation options are picked up. It does not yet reuse
a simplex basis or MIP start; current ASCEND values are not supplied as a start.
Currently an objective and nonempty solver variable/relation lists are required;
WHENs and logical relations are unsupported. Strict inequalities use the usual
closed LP inequality, as in HiGHS/MakeMPS. Algebraic cancellation is not attempted
by the conservative affine classifier.

## Shared architecture and tests

`ascend/system/lp_export.c` now provides preparation, sparse column packing,
objective offsets, affine classification, domain relaxation, LP/MIP classification
and solution writeback for both HiGHS
and Gurobi. These use the existing `lp_utils.c` matrix/bounds/type/scaling code.
Vendor-specific types, options, lifecycle, callbacks and status mapping stay in
each plugin. HiGHS retains its LP/MIP and tangent-LP capabilities.

```sh
scons -j4 WITH_GUROBI=1 test/test
./a4 cutest solver_highs solver_makemps
./a4 cutest solver_gurobi
ASCEND_TEST_GUROBI=1 python3 -m unittest discover -s test -p test_gurobi_listing.py
```

The ordinary Gurobi suite runs licensed solves whenever the plugin is available;
no enabling environment variable is needed. Missing/invalid licensing fails tests.
An absent plugin is skipped; `ASCEND_TEST_GUROBI=1` makes its absence a failure too.
The separate license-listing Python tests still require that variable.

Coverage includes AFIRO and the LP showcases, integer/binary/semi domains,
global/individual relaxations, integer-safe scaling, objective offsets, resolve,
failure statuses, progress/interrupts and early termination with an incumbent.
Parameter tests check defaults/ranges and invalid values without licensing, and
exercise non-default LP/MIP tuning plus work, memory, simplex and barrier limits.
The facility-location and eight-city travelling-salesperson fixtures in
`models/test/mip/` run under both Gurobi and HiGHS, including their labelled-TABLE
variants. Shared domain regressions live in `models/test/mip/domains.a4c`.

The existing Python progress driver can also compare either solver on a MIP:

```sh
./a4 script ascxx/testhighs.py models/test/mip/facility_location.a4c \
    --model mip_facility_location --solver Gurobi --no-tdlm
```

Use `--solver HiGHS` for the same problem with HiGHS. The known objective is 470.

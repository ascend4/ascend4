# Gurobi LP solver

Optional ASCEND solver registered as `Gurobi`. The first implementation solves
continuous LPs: affine objectives (minimize/maximize), equality/inequality rows,
and variable bounds. It does not yet expose Gurobi MILP, QP, or nonlinear models.

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

## Options and semantics

- `nonlin` (default false): explicitly request **one tangent LP** at the current
  model point. This is not an SQP iteration loop or a nonlinear solution. External
  relations require this option and working derivatives. Exact LP mode rejects
  black boxes and any expression whose affine form cannot be established.
- `relaxed` (default false): explicitly request the continuous relaxation of
  discrete solver variables. Individual `relaxed` variable flags are respected.
  Otherwise integer, binary and semicontinuous variables are rejected.
- `varnom_scale`, `relnom_scale` (default false): use the shared nominal scaling.
- `method`: -1 automatic, 0 primal simplex, 1 dual simplex, 2 barrier,
  3 concurrent, 4 deterministic concurrent.
- `presolve`: -1 automatic, 0 off, 1 conservative, 2 aggressive.
- `dual_reductions` (default true): disable and re-solve to distinguish
  infeasible from unbounded after an ambiguous `INF_OR_UNBD` result.
- `time_limit` (seconds), `threads` (0 automatic), `feasibility_tol`,
  `optimality_tol`, and `progress_callbacks`.

ASCEND presolve assembles the LP; Gurobi's own presolve runs during optimization.
No source model bounds are repaired silently. A feasible primal solution is
written back with scaling undone; fixed values are preserved and residuals are
refreshed. Infeasible/unbounded/no-solution outcomes do not overwrite variables.
The LP status includes the objective constant, primal feasibility and iteration
counts. `INF_OR_UNBD` is retained as ambiguous, not labelled proven infeasible.
Interrupts are checked even when progress reporting is disabled.

`resolve` rebuilds coefficients, bounds and offsets before solving, so changes
to fixed parameters are picked up. It does not yet reuse a simplex basis.
Currently an objective and nonempty solver variable/relation lists are required;
WHENs and logical relations are unsupported. Strict inequalities use the usual
closed LP inequality, as in HiGHS/MakeMPS. Algebraic cancellation is not attempted
by the conservative affine classifier.

## Shared architecture and tests

`ascend/system/lp_export.c` now provides preparation, sparse column packing,
objective offsets, affine classification and solution writeback for both HiGHS
and Gurobi. These use the existing `lp_utils.c` matrix/bounds/type/scaling code.
Vendor-specific types, options, lifecycle, callbacks and status mapping stay in
each plugin. HiGHS retains its LP/MIP and tangent-LP capabilities.

```sh
scons -j4 WITH_GUROBI=1 test/test
./a4 cutest solver_highs solver_makemps
./a4 cutest solver_gurobi
ASCEND_TEST_GUROBI=1 ./a4 cutest solver_gurobi
ASCEND_TEST_GUROBI=1 python3 -m unittest discover -s test -p test_gurobi_listing.py
```

The ordinary Gurobi suite checks eligibility without acquiring a license and
skips solves. Set `ASCEND_TEST_GUROBI=1` to require the installed plugin and run
licensed solves, including AFIRO, scaling, offsets, resolve, failure statuses,
interrupts and explicit relaxations. Missing/invalid licensing then fails tests.

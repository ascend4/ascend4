# Conditional Decomposition for CMSlv

## Motivation

CMSlv currently treats conditional models as globally coupled configuration
problems. It alternates between logical solving, nonlinear/optimization steps,
boundary handling, and global reanalysis when discrete variables used in
`WHEN` statements change.

This is correct for many conditional models, but it misses an important
structural case: a `WHEN` can become effectively equivalent to a `SELECT` once
its controlling discrete variable has already been determined by an upstream
block.

For example:

```ascend
regime IS_A boolean_var;

CONDITIONAL
    c: x >= 0;
END CONDITIONAL;

regime == SATISFIED(c);

WHEN(regime)
    CASE TRUE:
        y = f1(x);
    CASE FALSE:
        y = f2(x);
END WHEN;
```

If `x` and `regime` are solved in an earlier block, then the downstream
`WHEN(regime)` does not require a global conditional solve. It can be evaluated
as a fixed branch selection for later blocks. Conceptually, it has become a
`SELECT` over a known value.

The goal of conditional decomposition is to detect and exploit this structure.

## Status Snapshot

The shared mixed decomposition in `ascend/system/decomp.[ch]` is the intended
planning graph for CMSlv2. It is not just a GUI/reporting feature. The current
implementation provides:

- `slv_decomp_partition`: the conservative structural envelope, including
  active rows, inactive `WHEN` case rows retained through `in_when`, selector
  activation edges, boundary dependencies, real variables, discrete variables,
  relations, and logrelations;
- `slv_decomp_partition_active`: the current execution view, using only rows
  whose `included` and `active` flags are both true after current conditional
  analysis has selected active cases;
- CMSlv progress/reporting that classifies active execution blocks and
  conservative mixed envelopes into intended CMSlv2 sub-solver responsibilities;
- an opt-in `cmslv2` parameter that enables QRSlv's native real block
  partitioning at subsidiary-solver setup time.

Current executable behavior is still partial:

- active pure-real blocks can be handed to QRSlv via QRSlv's existing
  partitioning machinery when `cmslv2=TRUE`;
- active pure-logical blocks are identified, but are still solved by CMSlv's
  existing global LRSlv phase rather than by a block-local LRSlv dispatcher;
- selector-coupled, boundary/logical/real, integer, and unresolved mixed
  envelopes are classified for CMSlv2 planning, but still fall through the
  original CMSlv control flow.

The next direction is to replace the global CMSlv loop with a CMSlv2
planner/dispatcher loop that consumes the mixed decomposition directly:

1. process conservative structural blocks as scheduling envelopes;
2. reanalyse the active execution view on entry to each envelope;
3. dispatch active pure-real subblocks to QRSlv;
4. dispatch active pure-logical subblocks to LRSlv or a block-local logical
   propagation hook;
5. run selector enumeration plus re-analysis for selector-coupled envelopes;
6. run CMSlv boundary traversal for boundary/logical/real envelopes;
7. fall back to legacy full CMSlv only for unresolved large mixed or integer
   cases.

## Current Limitation

CMSlv explicitly disables QRSlv block partitioning while solving conditional
models:

```c
/*
    Disabling the partition mode flag in the nonlinear solver.
    PARTITION is a boolean parameter of the nonlinear solver
    QRSlv. This parameter tells the solver whether it should block
    partition or not.
    As long as we do not have a special subroutine to partition
    conditional models, this option should be disabled while using
    the conditional solver CMSlv
*/
```

The current solver does distinguish some trivial boundary situations. At a
boundary it perturbs relevant `SATISFIED(...)` terms, solves logical relations,
and checks whether any `WHEN`-participating boolean variables actually change.
If no configuration changes, the boundary is not considered a real
configuration boundary.

That is useful, but it is not block decomposition. It does not exploit the case
where upstream equations/logrelations determine a discrete value before
downstream `WHEN` cases are considered.

## Desired Behavior

CMSlv should build a mixed dependency structure containing:

- continuous variables;
- algebraic relations;
- discrete variables;
- logical relations;
- boundary relations used by `SATISFIED(...)`;
- `WHEN` instances and their case-local relations/logrelations.

Using this structure, the solver should identify conditional blocks in a
dependency order. If a discrete selector is determined before a later block is
assembled, then `WHEN` instances depending only on that selector can be reduced
to their active cases for the later block.

In effect:

```text
solve block A
  determines x and regime

collapse WHEN(regime) in block B to the active case

solve block B as an ordinary nonlinear block
```

This should reduce unnecessary global reconfiguration and avoid treating every
conditional model as a fully coupled mixed logical/nonlinear problem.

## Terminology

`configuration variable`
: A discrete variable that controls one or more `WHEN` instances.

`selector group`
: The ordered control list of a `WHEN`. This may be a single selector atom,
  such as a symbol-valued `selector`, or a tuple/list of discrete atoms such as
  `(bol1, bol2)`. The mixed decomposition should still store these as ordinary
  discrete-variable columns, because each member may have its own defining
  boundary or logical relation. Reports and solver interpretation should treat
  the group as the unit that selects a `CASE`.

`configuration boundary`
: A boundary relation whose truth value can change one or more configuration
variables through logical relations.

`conditional block`
: A block containing continuous and/or logical equations together with the
configuration variables and boundaries needed to determine its active equation
set.

`resolved selector`
: A configuration variable whose value is fixed by earlier blocks or by user
specification at the point a later block is considered.

`SELECT-like WHEN`
: A `WHEN` whose controlling variables are all resolved selectors. Only one
case can be active, so downstream solving can ignore inactive cases.

## Sketch Algorithm

1. Build the ordinary real incidence graph for variables and relations.

2. Build a logical incidence graph for discrete variables, logical relations,
   and `SATISFIED(...)` boundary dependencies.

3. Add activation edges from `WHEN` controlling variables to relations and
   logrelations inside their cases.

4. Add boundary edges from continuous variables in boundary relations to the
   logical relations that consume those boundaries.

5. Compute strongly connected components or block-triangular form over this
   mixed graph.

6. For each block in order:

   - identify discrete variables already fixed by user specification or solved
     in earlier blocks;
   - reduce any `WHEN` whose controlling variables are already resolved;
   - solve logical relations local to the block;
   - solve the continuous block with only currently active relations;
   - update boundary status and propagate changed discrete values to dependent
     downstream blocks.

7. If a nonlinear step crosses a configuration boundary belonging to the
   current or an earlier block, return to the boundary and re-solve from the
   earliest affected conditional block.

The implementation now needs two decomposition views:

- conservative structural view: all candidate conditional rows using
  `included && (active || in_when)`, plus selector-to-row activation edges;
- active execution view: only currently active rows using `included && active`,
  with selector-to-row edges omitted because selector values have already been
  consumed into row `active` flags.

`slv_decomp_partition` builds the conservative view. `slv_decomp_partition_active`
builds the active-row view and is the starting point for block re-analysis after
selectors or boundary truth values are resolved.

CMSlv now runs a non-invasive decomposition assessment at presolve and after
conditional reconfiguration. It also assesses the current active blocks before
QRSlv presolve. The progress event reports both `active_recommended`, meaning
the current active view is pure real plus pure logical, and `structural_safe`,
meaning the conservative conditional envelope is also free of selector,
boundary, integer, and mixed blocks. `recommended` is true only when both are
true.

The direct QRSlv `partition=1` handoff is disabled in legacy CMSlv and enabled
only by the opt-in `cmslv2` parameter. Two experiments matter:

- mutating QRSlv's `partition` parameter at CMSlv iteration time disrupted
  `linmassbal.a4c`;
- enabling QRSlv partitioning at subsidiary-solver setup is stable for
  `linmassbal.a4c` and is covered by `linmassbal_cmslv2`, but the earlier
  global experiment broke the `pipeline` CMSlv self-test.

The likely reason is solver-control granularity. QRSlv partition mode advances
one real block at a time, then CMSlv immediately performs global boundary
analysis. For models whose active view is pure-real but whose conservative
envelope still contains boundary/selector alternatives, that changes when CMSlv
reacts to boundaries. Therefore a correct block-local path must let CMSlv own
the conservative envelope, decide which boundary/logical state is resolved, and
then hand only safe real subblocks to the numeric solver. The assessment uses
the current `slv_system_t`, including the relation/logrelation `active` flags
propagated by existing conditional analysis, and reports how the conservative
structural view collapses into active execution block kinds. This is the
intended staging point for a future `cmslv2` execution path that dispatches
active subblocks while preserving the legacy CMSlv fallback.

An opt-in experimental `cmslv2` parameter now enables QRSlv's native real-block
partitioning at subsidiary-solver setup time. This is deliberately not the
legacy default and it is not the final CMSlv-owned envelope scheduler, but it
provides a concrete executable milestone: `linmassbal.a4c` solves with
`partition=1` and QRSlv reports four real blocks, while the ordinary CMSlv suite
continues to run with `partition=0`. The `pipeline.a4c` experiment remains the
warning case showing why this mode must stay opt-in until CMSlv controls the
conditional envelope around each numeric block.

The CMSlv2 planning event is now the explicit consumer of the mixed
decomposition. It reports:

- `qrslv_blocks`: active pure-real subblocks intended for QRSlv;
- `lrslv_blocks`: active pure-logical subblocks intended for LRSlv-style
  propagation;
- `enum_blocks`: conservative selector-coupled envelopes intended for local
  selector enumeration followed by active re-analysis and numeric solve;
- `boundary_blocks`: conservative boundary/logical/real envelopes intended for
  CMSlv boundary traversal;
- `integer_blocks` and `fallback_blocks`: unresolved cases that currently need
  a future integer/discrete subsolver or the legacy full CMSlv path.

This is the architecture we want, but only the QRSlv handoff is executable in
the current `cmslv2` switch. Pure logical blocks are still solved by CMSlv's
existing global LRSlv call, and selector/boundary/fallback envelopes are still
handled by the original CMSlv control flow. The next implementation step is to
replace those global phases with a CMSlv2 loop over these planned block
responsibilities.

## Mock CMSlv Outer Loop

The low-level decomposition should remain a generic mixed incidence structure.
The CMSlv controller should derive a block assessment from the live
`slv_system_t` state each time it enters a conservative block or downstream
region. This deliberately reuses the existing solver lists, active flags, and
matrix/block machinery rather than creating a second ownership model for
relations and variables:

```text
structural = slv_decomp_partition(system)
resolved = initially fixed vars/dvars and known boundary statuses

for each structural block or invalidated downstream region:
    propagate known logical values and selector groups
    update WHEN CASE active flags where selector groups are known
    active = slv_decomp_partition_active(system)
    local = restrict active view to the current structural envelope/downstream frontier
    subblocks = block-triangularize(local)

    for each subblock:
        kind = assess rows, columns, active flags, and known/fixed state
        result = dispatch_block_solver(kind, subblock)
        commit newly resolved vars, dvars, boundary statuses, and row active bits

    if a solve changes an upstream boundary truth value:
        mark dependent active flags and downstream results invalid
        restart from the earliest affected structural block
```

The key point is that a conservative block is an envelope, not necessarily the
unit sent to a solver. Entering the envelope may resolve enough row-active
status to split it into smaller active subblocks. Pruning can only remove graph
edges, but it can still reveal branch-specific underdetermined columns or rows
outside the square assigned region.

### Derived Block Solver Kinds

The "block type" should be a derived assessment, not a permanent identity stored
in the decomposition data structure.

- Empty/pruned block: all rows are inactive or already satisfied; skip and mark
  downstream dependencies clean.
- Pure real block: active real relations and real solver variables only; pass
  to QRSlv-style numeric block solving or CMSlv's real subproblem path.
- Pure logical block: active logrelations and boolean dvars only; pass to
  LRSlv-style propagation.
- Selector-resolved conditional block: selector group is known, so update row
  `active` flags, rebuild the active view, and then usually fall through to
  pure real or pure logical subblocks.
- Boundary/logical mixed block: selector values depend on local
  `SATISFIED(...)` boundaries. Use CMSlv's local boundary negotiation: propose
  or enumerate admissible selector tuples, solve the active real subblock, then
  verify the `SATISFIED` definitions and logical consistency.
- Integer or symbol selector block: if the selector is fixed or upstream-known,
  treat it like selector-resolved. If it must be chosen locally, use a future
  integer/discrete solver slice or fall back to the existing global CMSlv
  strategy.
- Large unresolved mixed block: use the existing CMSlv large-block machinery as
  the conservative fallback. This is not a regression; it is the original
  difficult case, just isolated from blocks that can be simplified.

## Important Cases

### Fixed Selector

If a `WHEN` variable is fixed before solve, the active case is known. This is
already close to `SELECT` behavior and should be treated cheaply.

### Upstream-Determined Selector

If a selector depends on variables solved in a prior block, then downstream
`WHEN` cases can be collapsed after that block. This is the main target of this
work.

### Local Conditional Block

If a selector and the continuous variables that determine its boundary are in
the same strongly connected component, the block remains genuinely conditional.
CMSlv must still use its boundary/logical machinery for that block.

### Downstream Boundary Crossing

If a later nonlinear step changes a variable that feeds an upstream or current
configuration boundary, the decomposition ordering was too optimistic for that
step. The solver must return to the relevant boundary and invalidate dependent
blocks.

### Boundary That Does Not Change Configuration

The existing "not really at a boundary" test remains useful. Conditional
decomposition should preserve it, but can make it more local by testing only
the configuration variables affected by the current block.

## Relationship To Syntax

This feature is solver-side. It should benefit existing models using:

```ascend
CONDITIONAL
    c: ...
END CONDITIONAL;

b == SATISFIED(c);

WHEN(b)
    CASE TRUE:
        USE eq1;
    CASE FALSE:
        USE eq2;
END WHEN;
```

It also provides a foundation for future higher-level syntax such as:

```ascend
IF r < 1 THEN
    delta = delta_slow;
ELSEIF r < 5 THEN
    delta = delta_intermediate;
ELSE
    delta = delta_fast;
END IF;
```

Such syntax could lower to conditional/logical machinery, while CMSlv uses
conditional decomposition to solve the resulting model efficiently when the
branch conditions are upstream-determined.

The hybrid/event `WHEN(mode) ... SWITCH TO ... IF ...` syntax is related but
not identical. Event transitions describe dynamic state changes. Conditional
decomposition is about steady-state admissibility and block-local equation
activation.

## Implementation Notes

- Start by recording dependencies between `WHEN` controlling variables and
  case-local relations/logrelations in a form usable by solver analysis.

- Reuse existing solver lists where possible:
  `slv_get_solvers_dvar_list`, `slv_get_solvers_logrel_list`,
  `slv_get_solvers_bnd_list`, and `slv_get_solvers_when_list`.

- The first useful implementation does not need to solve every mixed SCC
  optimally. A conservative algorithm is acceptable:

  - only collapse `WHEN`s whose selectors are fixed or already solved;
  - otherwise leave the block to existing CMSlv behavior;
  - never collapse across an unresolved boundary dependency.

- Add diagnostics explaining why a `WHEN` was or was not reduced:

  ```text
  CMSlv decomp: WHEN foo reduced to CASE TRUE; selector b resolved in block 2
  CMSlv decomp: WHEN bar remains conditional; selector q depends on local boundary c
  ```

- Keep the existing global path as a fallback until decomposition is mature.

## Current Implementation Status

Initial shared-system support now lives in `ascend/system/decomp.[ch]`. It is
now wired into CMSlv as reporting and as a conservative partition-policy
assessment. The pass deliberately reuses the same `mtx` symbolic assignment and
`mtx_region_t` block representation used by QRSlv's block partitioning, but it
does not reorder the live solver lists. It returns a separate
`slv_decomp_partition_t` containing:

- real relation rows;
- conditional relation rows;
- logical relation rows;
- conditional logical relation rows;
- continuous solver-variable columns;
- discrete-variable columns;
- block regions and row/column permutations;
- inverse row/column permutations for original-to-current lookup;
- recorded structural nonzeros for tests and diagnostics.

Two partition entry points are now available:

- `slv_decomp_partition` builds the conservative structural view, retaining
  case-local rows through `included && (active || in_when)` and adding
  selector-to-row activation edges;
- `slv_decomp_partition_active` builds the current execution view, using only
  `included && active` rows and omitting selector-to-row activation edges after
  `WHEN` cases have already been selected.

The first pass adds conservative mixed dependencies for:

- ordinary real relation to continuous variable incidence;
- logical relation to discrete variable incidence;
- `SATISFIED(...)` boundary edges from boundary real variables to consuming
  logical relations;
- logical-boundary edges where a boundary is itself a logrelation;
- `WHEN` selector edges from discrete selectors to case-local relations and
  logrelations.

Discrete selector handling is intentionally broader than LRSlv's logical block
partitioning. `slv_log_block_partition` remains boolean/logrelation-oriented,
while `slv_decomp_partition` includes boolean, integer, and symbol discrete
variables when they are used as activation selectors. Fixed boolean selectors
are not treated as coupling edges, which is the first step toward SELECT-like
collapse. Integer and symbol selectors are retained even when they are not
logical-solver unknowns.

Selector groups are not represented as separate block columns. A `WHEN (a,b)`
contributes activation edges from both `a` and `b` to the case-local rows, while
a symbol/integer selector contributes one activation column. This preserves the
ordinary matrix/block machinery and avoids hiding the fact that tuple members
can be defined by different upstream boundaries. A higher-level report or CMSlv
consumer should recover the `WHEN` control list when explaining case selection
or when enumerating admissible cases; for example, `(bol1, bol2)` is a single
case guard even though it occupies two dvar columns.

Another way to read a `WHEN` edge is as a dependency on the row's active status:
the selector group does not directly change the numeric variables in an inactive
case, but it does determine whether each case-local relation or logrelation is
active and therefore whether that row contributes its ordinary incidences. The
current implementation does not add a literal "row-active" column to the matrix;
it uses selector-to-row activation edges as the compact representation of that
dependency. This is the reason the conservative structural view keeps inactive
case rows visible, while a case-pruned execution view should use only rows whose
`included` and `active` flags are both true.

These mixed blocks are analysis/scheduling blocks, not solver execution blocks.
A mixed block may contain real variables, discrete variables, real relations,
and logrelations when a boundary or selector prevents independent ordering.
Block type names such as "selector-coupled" or "boundary/logical" should remain
reporting and explanation terms derived from the row/column kinds already in the
block. They do not need to become extra block identities in the decomposition
data structure.
Before dispatch to a concrete solver, CMSlv should project the mixed block into
typed subviews:

- real-only relation/variable subblocks for QRSlv or CMSlv numeric work;
- logical-only logrelation/boolean subblocks for LRSlv-style propagation;
- future integer/MIP slices for integer solver variables.

The mixed block is therefore the dependency boundary: it says the typed pieces
inside the block cannot be scheduled independently without negotiating the
activation or boundary dependency. This also preserves QRSlv's requirement that
its block list contain only real variables and real relations.

`analyze_make_solvers_lists` now also marks `VAR_INTEGER`, `VAR_BINARY`, and
`VAR_SEMICONT` on solver variables. This is useful for decomposition consumers
that need to distinguish continuous NLP variables from MIP-style solver vars
without re-querying compiler types.

The METHOD syntax `FIX b := TRUE` now works for discrete atoms with a boolean
`fixed` child, not just refined `solver_var` real atoms. This removes the need
to use the older direct `b.fixed := TRUE` form in decomposition fixtures and
future conditional-model tests.

Systematic CUnit coverage has been added in `ascend/solver/test/test_decomp.c`
with fixtures in `models/test/decomp/block_cases.a4c`. The covered cases are:

1. pure real chain decomposition compared with QRSlv block count;
2. real boundary to logical relation dependency through `SATISFIED`;
3. boolean `WHEN` selector activation edges;
4. fixed boolean selector not creating a coupling edge;
5. integer `WHEN` selector activation edges;
6. solver integer variables participating in ordinary real relations;
7. conditional relation and conditional logrelation row classification;
8. logrelations used inside `WHEN` cases;
9. public row/column kind helpers and null-input handling.

A focused GCOV run on the current decomposition/CMSlv work reports:

- `ascend/system/decomp.c`: 90.56% line coverage;
- `ascend/solver/test/test_decomp.c`: 96.35% line coverage;
- `solvers/cmslv/cmslv.c`: 72.69% line coverage;
- `ascend/solver/test/test_cmslv.c`: 86.67% line coverage.

The remaining uncovered `decomp.c` lines are mostly defensive allocation/error
paths, fallback overcoupling paths for unmatched `WHEN` ownership, and the
logical-boundary branch. The last two are real follow-up targets when the
analysis layer exposes tighter case-local ownership and when we add a fixture
for logrelation boundaries.

ASCXX now exposes a small reporting API through `IncidenceMatrix`:

- `getDecompBlockReport(block)`;
- `getDecompReport()`.
- `getActiveDecompReport()`.

The report lists each diagonal mixed block with row/column ranges, relation and
logrelation text, variables/discrete variables, current values, and the rows or
columns outside diagonal blocks. The report is deliberately descriptive: labels
such as selector-coupled or boundary/logical are explanations inferred from the
generic mixed incidence structure, not additional solver semantics encoded in
the block representation. This is intended for GUI/export diagnostics and for
solver-development scripts, for example:

```sh
./a4 script python3 - <<'PY'
import ascpy
L = ascpy.Library()
L.load('linmassbal.a4c')
M = L.findType('linmassbal').getSimulation('sim', True)
M.build()
im = M.getIncidenceMatrix()
im.getDecompIncidenceData()
print(im.getDecompReport())
PY
```

### `linmassbal.a4c` observations

`models/linmassbal.a4c` is a useful CMSlv integration example because the mixed
decomposition is much smaller than the raw candidate system. The current pass
finds 59 candidate rows, 38 candidate columns, 161 mixed incidences, and 7
diagonal blocks. Only the rank-25 diagonal region is assigned to dependency
blocks; rows outside the blocks are mostly inactive `WHEN` alternatives,
conditional boundary rows, and logical rows that do not currently drive an
assigned block. Columns outside the blocks are fixed feed/bound variables.

The blocks separate into two qualitatively different classes:

- selector-coupled variant blocks, such as units 1-3, contain real relations,
  real variables, and selector dvars, but no logrelations or boundary edges;
- true mixed boundary blocks, such as units 5, 6, and part of unit 4, contain
  `SATISFIED(...)` logrelations, boundary dependencies, selector dvars, and real
  equations in the same block.

This distinction matters for CMSlv. Selector-coupled blocks should often be
reducible to ordinary real solves once selector values are known. True mixed
boundary blocks need local CMSlv-style negotiation: logical propagation,
boundary evaluation, active-case selection, and then real solving of the active
relation subset. The mixed block should therefore be interpreted as a scheduling
boundary, not as a single monolithic solver call.

For `linmassbal`, a future block-CMSlv path should be able to work on small
local problems instead of the full model. A conservative execution strategy is:

1. Process blocks in mixed-decomposition order.
2. For selector-only blocks, use current/resolved selector values to select the
   active case, then pass only the active real relation/variable subblock to the
   real solver.
3. For blocks with logrelations or boundary edges, run local logical/boundary
   analysis first, then solve the active real subblock.
4. Treat rows outside diagonal blocks as diagnostics or deferred/inactive
   structure unless a boundary crossing makes them active.

The current active-row experiment on `linmassbal.a4c` shows the intended
direction: the conservative view has 7 structural blocks and 161 incidences,
while the current active view has 16 execution blocks and 51 incidences. In
other words, pruning `WHEN` alternatives can split the structural envelope into
smaller current subblocks. The largest active block still contains 10 real
relations and 10 real variables, so large-block CMSlv work remains necessary,
but the active view isolates many logical selector updates as one-logrel/one-dvar
blocks and removes inactive variant equations from the real solve path.

The current conservative fallback for `REL_INWHEN`/`LOGREL_INWHEN` rows may add
selector edges from all solver `WHEN`s if exact case-local object matching is
not available. This is structurally safe but may overcouple models with multiple
independent integer/symbol `WHEN`s. Before CMSlv uses the pass for performance
decisions, this should be tightened by exposing or normalising the case-local
relation/logrelation ownership data during analysis.

## Testing Plan

1. Fixed selector:

   - `WHEN(b)` with `FIX b`;
   - verify only active branch is solved and result matches the existing
     global CMSlv path.

2. Upstream selector:

   - block A solves `x`;
   - logrelation computes `b == SATISFIED(x >= 0)`;
   - block B uses `WHEN(b)`;
   - verify block B is reduced after block A.

3. Local conditional block:

   - `x` is solved by equations inside the `WHEN`-dependent block;
   - verify the solver leaves it as a CMSlv conditional block.

4. Boundary crossing:

   - construct a step where a downstream solve crosses a boundary used by a
     selector;
   - verify invalidation/re-solving starts at the earliest affected block.

5. No configuration change at boundary:

   - boundary reaches zero but perturbation does not change any active case;
   - verify the boundary is ignored as non-configurational.

## Open Questions

- Should conditional decomposition be a CMSlv-only pass, or should the mixed
  incidence graph live in the shared solver system layer?

  Initial answer: the mixed graph now lives in the shared system layer. This
  should also make it available to A4SQP-style pre/post solve analysis without
  importing CMSlv internals.

- How should inactive `WHEN` case equations be represented during block
  analysis so that inactive branches do not create false continuous
  dependencies?

- Can logical blocks be solved independently with LRSlv, or does LRSlv need
  block-local solve support?

- How much of QRSlv partitioning can be reused once `WHEN` activation has been
  resolved for a block?

  Initial answer: the matrix assignment and block-region machinery can be
  reused directly. Directly switching on QRSlv's live `partition` parameter
  inside CMSlv is not safe yet for conditional models, even when the active
  mixed decomposition looks pure-real plus pure-logical. `linmassbal.a4c` and
  `pipeline.a4c` both currently report `active_recommended=1` and
  `structural_safe=0`: current active rows can be split into real/logical blocks,
  but the conservative envelope still contains conditional structure. Live
  solver-list reordering should therefore wait until CMSlv has a block-local
  execution path, because the mixed graph has more row/column kinds than QRSlv's
  real-only lists and CMSlv needs to preserve row-active propagation around each
  handoff.

- What user-facing diagnostics are needed to make decomposition decisions
  understandable?

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
- CMSlv2 progress/reporting that classifies active execution blocks and
  conservative mixed envelopes into intended CMSlv2 sub-solver responsibilities;
- CMSlv2 now lives as a separate loadable solver in `solvers/cmslv2`. Legacy
  CMSlv has been restored to the pre-decomposition path, with the IPOPT and
  CONOPT regression tests remaining in `solver_cmslv`;
- a QRSlv `external_blocks` option that lets QRSlv trust an already-installed
  solver block list instead of running its own block partitioner.

Current executable behavior is partial but no longer just diagnostic:

- active pure-real execution blocks are delegated to QRSlv only when they are
  due at the current conservative structural-block cursor. CMSlv2 installs the
  consecutive due pure-real block run into the live solver rel/var arrays,
  temporarily scopes row/column active flags to that run, disables QRSlv's own
  `partition` mode, and enables QRSlv's `external_blocks` mode. QRSlv is
  therefore used as a numeric block solver while the mixed block planning still
  comes from `slv_decomp_partition*`;
- selector-coupled structural envelopes run through a first flat selector
  handoff routine that confirms the currently active `WHEN` cases, rebuilds the
  active decomposition, and reports the resulting QRSlv/LRSlv/unresolved
  subblocks;
- boundary/logical/real envelopes have an explicit CMSlv2 handoff routine that
  records delegation to the existing full CMSlv boundary traversal. In addition,
  the scheduler now has a first due-block boundary dispatcher,
  `event=cmslv2_boundary_local`, that recognises a due boundary-mixed
  structural block, temporarily scopes rel/condrel/logrel/condlogrel and
  var/dvar active flags to that block, rebuilds the scoped active view, and
  reports the local rows/columns/subblocks it would expose. The dispatcher now
  also calls the existing boundary discovery and boundary optimization/return
  routines while the scoped view is installed, reported as
  `event=cmslv2_boundary_local_traverse`; when that local traversal succeeds,
  it captures the accepted real values, discrete values, and boundary flags,
  restores the temporary scope, then replays the accepted result into the full
  system and runs the normal conditional reanalysis so active/inactive row
  outcomes are propagated by existing solver-list logic. CMSlv2 now also invokes
  this policy from the `boundary_at_zero` path, so a model that starts on a due
  boundary-mixed envelope can exercise the same local accept machinery before
  the legacy full-boundary fallback continues. That `boundary_at_zero` use
  suppresses QRSlv external-block installation, because the caller is not about
  to transfer control to QRSlv; it may still consume newly exposed LRSlv blocks;
- the mixed-block scheduler policy now has one explicit transition rule:
  commit the block solver result, certify that the mixed block is solved, then
  advance the structural cursor and rebuild the structural/active view from the
  live `slv_system_t`. This active-structure reanalysis is only needed after
  mixed blocks that can change row active/inactive state; pure QRSlv/LRSlv
  blocks do not pay that decomposition cost just for advancing the cursor.
  QRSlv handoff is also an explicit caller policy: either install a due
  pure-real run for an immediate QRSlv call, or suppress QRSlv scope changes
  when the current legacy CMSlv path is not about to invoke QRSlv;
- when QRSlv handoff is suppressed by the caller, the scheduler can now consume
  newly exposed due pure-real structural runs locally using the same save,
  install, solve, restore discipline as the selector-local QR helper. This is
  reported as `event=cmslv2_qrslv_local`. It is deliberately not yet used to
  skip the legacy boundary fallback unless the boundary envelope also passes
  `event=cmslv2_boundary_local_validate`;
- boundary-local acceptance now has two levels. `accepted` means a scoped
  candidate was captured and can be tested against the live system.
  `local_complete` means the current boundary envelope passed the first
  conservative completion certificate: active real rows in the envelope satisfy
  residual tolerance after propagation, and the active decomposition inside the
  envelope has no unresolved subblocks. Only `local_complete=1` advances the
  structural cursor or bypasses legacy `boundary_at_zero` fallback. An accepted
  but uncertified candidate is reverted before the legacy fallback continues;
- active pure-logical structural runs can now be grouped and delegated to LRSlv
  with `external_blocks=TRUE`, using an installed logical block list in the same
  spirit as the QRSlv pure-real handoff;
- integer, symbol-selector, and unresolved mixed envelopes are still classified
  only, then handled by the original CMSlv control flow.

This is not yet a fully block-dispatching CMSlv2. The missing core is a
planner/dispatcher loop that consumes the conservative structural blocks as the
primary solve schedule:

1. process conservative structural blocks as scheduling envelopes;
2. reanalyse the active execution view when entering each envelope;
3. when the cursor reaches one or more consecutive conservative pure-real
   structural blocks, install only that due run into the live solver rel/var
   lists and dispatch it to QRSlv with `external_blocks=TRUE`;
4. dispatch consecutive due active pure-logical subblocks to LRSlv with an
   explicit logical row/column scope;
5. for selector-coupled envelopes, branch only over unresolved selector groups,
   rebuild the active view after each candidate, solve the resulting real/logical
   subblocks, and verify consistency;
6. for boundary/logical/real envelopes, restrict CMSlv boundary traversal to the
   affected boundaries, dvars, active real rows, and invalidated downstream
   blocks where that locality is provable;
7. invalidate and reanalyse downstream regions when a solved boundary changes a
   row-active flag;
8. fall back to legacy full CMSlv only for unresolved large mixed, integer, or
   non-local boundary cases.

## Implementation Review

What we have now:

- shared mixed structural and active decompositions in `ascend/system/decomp`;
- ASCXX reporting and incidence-matrix visualisation support for structural and
  active block views;
- CMSlv2 progress events for decomposition assessment and CMSlv2 planning;
- a separate `CMSlv2` solver package, so legacy `CMSlv` can continue to cover
  the IPOPT/CONOPT paths without carrying the block-decomposition scheduler;
- QRSlv `external_blocks=TRUE`, covered by `solver_qrslv.external_blocks`,
  which proves QRSlv can solve with a preinstalled block list when its own
  `partition` parameter is false;
- QRSlv single-block scoping, covered by
  `solver_qrslv.external_single_block_scope`, which proves that a one-block
  installed list can make QRSlv treat that block as the current full real
  problem;
- CMSlv2 structural-cursor scheduling for QRSlv: consecutive due conservative
  pure-real structural blocks are installed for QRSlv, while mixed blocks stop
  the QRSlv handoff and are reported as `action=defer_subsolver`;
- a flat selector-envelope diagnostic sweep, reported as `event=cmslv2_enum`
  only when `progress_log=TRUE`. Normal CMSlv2 execution now uses the
  cursor-aware selector consumer directly, because the all-envelope sweep was
  repeatedly rebuilding active partitions without affecting the scheduler's
  actual block decisions;
- a first cursor-aware selector-envelope consumer, reported as
  `event=cmslv2_selector_consume`, that advances the structural cursor when the
  current selector envelope is already resolved by current selector values and
  active re-analysis leaves no unresolved subblocks;
- a full-CMSlv boundary handoff routine. Normal progress callbacks now report a
  lightweight `event=cmslv2_boundary_summary diagnostic=0` marker; the heavier
  per-boundary-envelope decomposition summary is retained for
  `progress_log=TRUE`;
- LRSlv `external_blocks=TRUE`, covered by `solver_lrslv.external_blocks`,
  which proves LRSlv can solve an installed logical block list without running
  its own logical block partitioner;
- CMSlv2 structural-cursor scheduling for LRSlv: consecutive due conservative
  pure-logical structural blocks are installed for LRSlv and reported as
  `event=cmslv2_lrslv_handoff`;
- CUnit coverage for mixed decomposition fixtures and for the `linmassbal`
  CMSlv2 progress path;
- CMSlv2 CUnit variants for the original CMSlv model set:
  `linmassbal_cmslv2`, `pipeline_cmslv2`, `heatex_cmslv2`, and
  `reinitignore_cmslv2`. All four currently pass their legacy `self_test`
  methods with `cmslv2=TRUE`. The `heatex` discrepancy exposed an important
  selector-search bug: CMSlv2 was accepting the current selector mask before
  the boundary/logical truth implied by the candidate solve had been refreshed.
  Selector candidate validation now updates boundaries, solves logical
  relations, and rejects candidates whose resolved selector mask differs from
  the assumed mask;
- a profiling pass over the original CMSlv regression models. With 3 warmups
  and 20 measured repeats, timing only `M.solve(...)`, CMSlv2 is still slower
  than legacy CMSlv on these small examples, but diagnostic gating reduced the
  overhead substantially. Current medians are approximately: `linmassbal`
  11.4 ms legacy vs 26.6 ms CMSlv2, `pipeline` 98.4 ms vs 192.9 ms, `heatex`
  11.7 ms vs 49.1 ms, and `reinitignore` 2.3 ms vs 3.1 ms. The earlier
  prototype medians were roughly 47.2 ms, 495.2 ms, 84.8 ms, and 3.9 ms for
  CMSlv2 respectively, so most of the easy win came from removing redundant
  diagnostic repartitioning. Transition and boundary-local scoped active
  partition details are also now `progress_log=TRUE` diagnostics rather than
  normal scheduler work. CMSlv2 is not yet an acceleration path for these small
  models; the expected payoff still depends on larger decomposable systems and
  stronger local mixed-block completion;
- a deterministic CMSlv2 scheduler fixture in
  `models/test/cmslv/cmslv2_scheduler.a4c`, covered by
  `solver_cmslv2.cmslv2_scheduler`, that asserts the observed scheduler sequence:
  selector branch search, QRSlv pure-real handoff, legacy boundary traversal,
  another selector branch search, grouped LRSlv pure-logical handoff, then
  scheduler fallback after all structural blocks have been consumed.
- a deterministic structural boundary-mixed fixture,
  `boundary_mixed_cycle` in `models/test/decomp/block_cases.a4c`, covered by
  `solver_decomp.boundary_mixed_cycle_block`. It creates the local cycle
  `b = SATISFIED(x = q)`, `WHEN(b) USE x + y = 2`, plus `q = y`, and asserts
  that the conservative block contains real rows, a logrel row, real columns,
  a dvar column, a boundary edge, and a selector edge.
- a CMSlv2 boundary-local fixture,
  `models/test/cmslv/cmslv2_boundary_local.a4c`, covered by
  `solver_cmslv2.cmslv2_boundary_local`, that starts at a due boundary-mixed
  envelope and asserts `event=cmslv2_boundary_local_commit action=accepted`
  followed by `event=cmslv2_block_solved solved=0` and
  `event=cmslv2_boundary_local_commit action=reverted`, proving that an
  uncertified mixed-block candidate does not advance the cursor to downstream
  LRSlv/QRSlv work.
- a positive CMSlv2 boundary-local fixture,
  `models/test/cmslv/cmslv2_boundary_local_complete.a4c`, covered by
  `solver_cmslv2.cmslv2_boundary_local_complete`, that starts at a due
  boundary-mixed envelope whose active real/logical rows are already
  satisfiable locally. It asserts `event=cmslv2_block_solved solved=1`,
  `event=cmslv2_transition solver=boundary`, cursor advancement, a downstream
  LRSlv handoff, a downstream local QRSlv run, and final
  `event=cmslv2_scheduler action=local_complete`.

What is still missing:

- block subsolvers that can advance the structural cursor through unresolved
  selector cases and boundary envelopes. Boundary envelopes are now detected,
  scoped locally, traversed through the existing CMSlv boundary routines, and
  locally certified when the completion predicate passes. Uncertified
  candidates are reverted and left for legacy fallback; richer fallback reasons
  for non-local boundary envelopes still need to be made explicit;
- true selector enumeration/search for unresolved selector groups, including
  multiple admissible cases and failure/backtracking policy;
- a stronger success contract for boundary-local solves. The scheduler now
  reports explicit boundary-local defer reasons (`scope_failed`,
  `not_at_boundary`, `optimize_failed`, `capture_failed`,
  `not_local_complete`) and validates local completion with current-envelope
  residual and unresolved-active-subblock checks. The positive fixture proves
  the current contract can advance through downstream LRSlv/QRSlv work; the
  negative fixture still reports `reason=residual_not_satisfied` and rolls
  back to legacy fallback;
- a result contract for each block solver: solved variables/dvars, activated or
  pruned rows, invalidated downstream blocks, residual status, and fallback
  reason;
- a freshness contract for each block solver: before returning success or
  asking CMSlv2 to certify completion, it must ensure the residual values for
  its active real rows and the satisfied flags for its active logical rows are
  current in the live `slv_system_t`;
- integer/symbol discrete subsolver support;
- systematic tests that prove block-local dispatch solves the same models as
  legacy CMSlv while solving smaller active subproblems.

`event=...` names in this document are progress-message labels emitted through
CMSlv's existing progress callback. They are test and diagnostic markers, not a
separate event system or a state-machine transition. Similarly, "handoff
routine" means a thin CMSlv2 control point that identifies the block kind and
delegates to existing solver machinery; it is not yet a block-local solver in
the stronger sense.

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

The direct QRSlv `partition=1` handoff remains disabled in CMSlv, including
CMSlv2. Three experiments now matter:

- mutating QRSlv's `partition` parameter at CMSlv iteration time disrupted
  `linmassbal.a4c`;
- enabling QRSlv partitioning at subsidiary-solver setup was stable for
  `linmassbal.a4c` in an earlier experiment, but the global approach is no
  longer used because it broke the `pipeline` CMSlv self-test;
- QRSlv `external_blocks=TRUE` with `partition=FALSE` is covered by
  `solver_qrslv.external_blocks` and proves that QRSlv can consume a block list
  installed before QRSlv presolve.

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
intended staging point for the `CMSlv2` execution path that dispatches active
subblocks while preserving the legacy CMSlv fallback.

CMSlv2 now uses this narrower QRSlv path directly. Before QRSlv presolve,
CMSlv2 builds the active mixed decomposition, verifies that all active real rows
and columns are covered by installable pure-real blocks, reorders the live
solver rel/var arrays so those blocks occupy the front of QRSlv's view,
installs the block list with `slv_set_solvers_blocks`, and invokes QRSlv with
`partition=FALSE` and `external_blocks=TRUE`.

This lets CMSlv2 own the mixed planning and row-active propagation while
reusing QRSlv's matrix setup, per-block reordering, and Newton iteration
machinery for real numeric blocks. The current implementation uses a structural
cursor and will not jump over a mixed envelope to solve downstream real blocks.
For `linmassbal`, the first due structural block is still mixed, so CMSlv2
reports `event=cmslv2_scheduler action=defer_subsolver` and falls back to the
existing mixed/boundary path. Once a mixed subsolver advances the cursor, the
same QRSlv handoff can solve the following run of one or more pure-real blocks.

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

The current `cmslv2` switch has three executable pieces. First,
`solver_decomp`/`slv_decomp_partition_active` classifies active real-only
execution blocks and CMSlv2 installs those blocks for QRSlv using
`external_blocks=TRUE`. This is no longer QRSlv rediscovering real-only blocks
globally; QRSlv consumes the block list chosen by CMSlv2. Second,
selector-coupled structural envelopes run through both a flat reporting pass
and a cursor-aware consumer. The reporting pass collects the dvars in each
selector envelope, uses the existing conditional case-matching helpers to
confirm the active case set for current selector values, rebuilds the full
active solver lists with `reanalyze_solver_lists`, rebuilds the active mixed
decomposition, and reports how the envelope collapses into QRSlv/LRSlv or
unresolved active subblocks through `event=cmslv2_enum`. The consumer applies
the reduction only at the current structural cursor. It now performs a bounded
boolean branch search (`event=cmslv2_selector_search`): current selector values
are tried first, then alternative boolean assignments are enumerated, each
candidate rebuilds the active solver lists and active decomposition, and the
first assignment that reduces the current structural envelope to resolved
active pure-real/pure-logical subblocks is passed through the local dispatch
path before it is committed. For reduced pure-real subblocks, CMSlv2 installs
only those subblocks for QRSlv using `external_blocks=TRUE`, runs QRSlv, then
restores the solver row/column order so rejected branches do not invalidate the
saved structural decomposition. The progress event is
`event=cmslv2_selector_qrslv`. Reduced pure-logical subblocks now follow the
same pattern with LRSlv: CMSlv2 installs only the local logrel/dvar blocks,
sets LRSlv `external_blocks=TRUE`, runs LRSlv, and restores logical solver
ordering and active flags afterwards. The progress event is
`event=cmslv2_selector_lrslv`. Non-boolean selectors and large selector groups
still defer to fallback handling.

The CMSlv2 scheduler can also consume consecutive due pure-logical structural
blocks before returning to the next block type. It groups that run, dispatches
it to LRSlv as one local request (`event=cmslv2_lrslv_handoff`), re-analyses
active rows and columns, then resumes scheduling QRSlv, selector, boundary, or
fallback work from the updated structural cursor.

The scheduler implementation deliberately treats reanalysis as central
bookkeeping rather than as part of the preceding block solver. A successful
block-local solver is responsible for committing values, dvar truth values,
boundary flags, and active/inactive row outcomes into the live `slv_system_t`.
After that commit, CMSlv2 rebuilds the conservative and active partitions and
then consumes any newly due LRSlv blocks before considering QRSlv handoff. In
call paths where QRSlv scope installation is suppressed because the caller is
not about to invoke QRSlv, CMSlv2 can now run due pure-real structural runs
locally and restore solver row/column order afterwards. This keeps the rule
simple: the next block is interpreted only through the current system state and
current structural cursor, not through special knowledge of whether the
previous block was selector-resolved or boundary-local.

Boundary-local completion is now deliberately distinct from boundary-local
candidate capture. A captured candidate can be useful evidence for the
boundary-local subsolver, but it is not by itself a proof that the boundary
episode is complete. After replaying the candidate into the full system, CMSlv2
evaluates active real rows in the current structural envelope and rebuilds the
active decomposition for that envelope. If any active real residual is above
tolerance or an unresolved active subblock remains, the candidate is reported as
`local_complete=0`, the pre-probe live state is restored, and the legacy CMSlv
boundary path is retained.

The completion predicate is intentionally a certification check, not a hidden
solver pass. It updates boundaries and evaluates active real residuals directly,
but it treats logical satisfaction flags as solver-maintained state. The block
solver contract is therefore that a successful block solve has already refreshed
the residual/satisfaction state for the active rows it owns before CMSlv2 asks
whether the block is currently solved.

Boundary/logical/real envelopes now have an explicit CMSlv2 handoff routine as
well.
When the legacy solver reaches either the "at boundary" optimization path or
the "boundary crossed" return path, CMSlv2 identifies the boundary-mixed
structural envelopes and reports a handoff with
`mode=full_cmslv_boundary`. The actual sub-solver is still the existing full
CMSlv boundary machinery: `at_a_boundary`, `optimize_at_boundary`,
`return_to_first_boundary`, boundary updates, and active-list re-analysis.

This is still not a complete mixed-block subsolver. The selector search now
validates reduced pure-real subblocks with QRSlv and reduced pure-logical
subblocks with LRSlv before accepting the branch. Nested boundary cycles are
not solved recursively. Boundary-local dispatch now scopes the existing CMSlv
boundary traversal to the current structural envelope, can commit an accepted
local result, and can immediately consume downstream LRSlv and QRSlv runs.
Fallback policy is still conservative: scope failure, no detected local
boundary, failed local optimization, failed result capture, non-converged local
QRSlv, residual failure in the current boundary envelope, unresolved active
subblocks, and any non-local boundary ambiguity continue through the legacy
full-boundary path. The next implementation step is to broaden the completion
certificate beyond active real residuals, especially around logrel/boundary
truth consistency and non-local boundary-effect detection.

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
now consumed by CMSlv2 as reporting and as a conservative partition-policy
assessment. The pass deliberately reuses the same `mtx` symbolic assignment and
`mtx_region_t` block representation used by QRSlv's block partitioning, but it
does not reorder the live solver lists itself. It returns a separate
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
  reused directly. QRSlv now has `external_blocks=TRUE`, which skips QRSlv's own
  structural partitioning and trusts the installed solver block list. Directly
  switching on QRSlv's live `partition` parameter inside CMSlv is still not safe
  enough for conditional models, even when the active mixed decomposition looks
  pure-real plus pure-logical. `linmassbal.a4c` and `pipeline.a4c` both
  currently report `active_recommended=1` and `structural_safe=0`: current active
  rows can be split into real/logical blocks, but the conservative envelope still
  contains conditional structure. CMSlv2 should therefore install only the
  active pure-real region that it is about to solve, set `external_blocks=TRUE`,
  and let CMSlv retain ownership of row-active propagation around the handoff.

- What user-facing diagnostics are needed to make decomposition decisions
  understandable?

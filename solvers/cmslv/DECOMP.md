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

- How should inactive `WHEN` case equations be represented during block
  analysis so that inactive branches do not create false continuous
  dependencies?

- Can logical blocks be solved independently with LRSlv, or does LRSlv need
  block-local solve support?

- How much of QRSlv partitioning can be reused once `WHEN` activation has been
  resolved for a block?

- What user-facing diagnostics are needed to make decomposition decisions
  understandable?


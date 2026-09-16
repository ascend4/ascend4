# Declarative FOR distribution

`FOR … CREATE` can combine declarations and dependent structural assignments:

```ascend
FOR i IN nodes CREATE
    n[i] IS_A integer_constant;
    n[i] :== count[i];
    unit[i] IS_A component(n[i]);
END FOR;
```

The compiler treats the three statements as independently pending loop nests,
as if each had its own `FOR i IN nodes CREATE`. They need not be written in
dependency order. The ordinary phase-1 retry mechanism resolves the dependence
of `unit[i]` on `n[i]`; no per-iteration scheduler or dependency graph is added.

This also supports nested/ragged structures: create an array of connection
sets, assign those sets, then create and configure elements indexed by them,
all within one source loop. TABLE can similarly populate a target constructed
in the same loop. Whole-array declarations remain preferable when they express
the desired rectangular structure more simply.

## Implementation and invariants

`DistributeCreateStatements` in `typedef.c` normalises model-body loops before
SELECT flattening and inheritance. Each phase-1 statement retains its enclosing
index names and expressions. New loop and SELECT wrappers retain source module,
line and context; their cached contents flags and SELECT counts are recomputed.
Leaf statements are reference-counted, not deep-copied. Compiled statement
listings can therefore show several loops originating from one source loop.

Each construction loop still prechecks and constructs its **whole domain** in
one execution. This preserves existing sparse-array completeness assumptions
in name lookup and model-parameter validation. A completed statement is handled
by the existing pending-statement bit list, rather than replayed on each retry.

Later-phase statements stay together and in their original order, including
overlapping `:=` defaults, equations and logical relations. Pure later-phase
loops are unchanged. Methods (`FOR … DO`), WHERE clauses, WHEN case contents and
CONDITIONAL relation blocks are not distributed. SELECT case bodies are
normalised before their flattened statement counts are calculated.

## Boundaries

Distribution resolves dependencies **between source statements**, not between
iterations of one statement. For example, this constant recurrence still waits:

```ascend
a[1..3] IS_A integer_constant;
a[1] :== 1;
FOR i IN [2..3] CREATE
    a[i] :== a[i-1] + 1;
END FOR;
```

The loop's precheck cannot yet evaluate every right-hand side. General support
would require finer-grained execution tracking, and arbitrary incremental
sparse construction would also require explicit array-completion tracking.
Genuine circular definitions remain unsatisfied. Existing rules for scalar
subscripts in sparse declarations and legal declarative statements still apply.

Regression coverage is in [the distribution tests](test/test_for_distribution.c)
and [model fixtures](../../models/test/instantiate/for_distribution.a4c), with
same-loop TABLE coverage in [the TABLE tests](test/test_tables.c).

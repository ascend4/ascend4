# DERIV notes

## Context

Branch: `ida-der-child-bug`

Primary repro:

- model: `models/test/ida/nested_array_independent.a4c`
- tests:
  - `integrator_ida.initial_hier_array_decay_build_failure`
  - `integrator_ida.initial_hier_array_decay_same_t_build_failure`
  - `integrator_ida.initial_hier_array_decay_param_t_build_failure`

Commands used:

```sh
./a4 cutest integrator_ida.initial_hier_array_decay_build_failure -v
./a4 cutest integrator_ida.initial_hier_array_decay_same_t_build_failure -v
./a4 cutest integrator_ida.initial_hier_array_decay_param_t_build_failure -v
```

## Current observed behaviour

### Case 1: child models each declare their own `INDEPENDENT t`

Current output includes:

- internal compiler/debug language:
  - `Adding declarative link`
  - `The LINK entry to-be added is already present in the declarative LINK table.`
- user-facing analysis errors:
  - `Set the same independent variable for all derivative chains!`
  - `ODE model requires exactly one INDEPENDENT variable; found 0.`

Observations:

- The `declarative link` wording is compiler-internal and not useful to an end-user.
- The duplicate-LINK warning is not actionable as currently phrased.
  It does not say whether the duplicate is benign, deduplicated, or fatal.
- The final diagnostics are contradictory from the user's point of view:
  - first we say there are multiple independent variables that must be merged
  - then we say zero independent variables were found
- The most likely interpretation is that unmerged child independents are detected in one path, but the later root-level validation path fails to count them coherently.

### Case 2: child `t` variables are merged with `ARE_THE_SAME`

Current output:

- `ODE model requires exactly one INDEPENDENT variable; found 0.`

This means the merged case still fails in the current tree.
So the answer to "does `ARE_THE_SAME` make it work?" is currently: no.

This case is important enough to keep as an explicit regression, because it is the
most reasonable modelling change a user would make after being told that multiple
independent variables must be merged.

Verified with:

```sh
./test/test -v integrator_ida.initial_hier_array_decay_same_t_build_failure
```

### Case 3: one shared `t` is passed into each child as a model argument

Current output:

- `ODE model requires exactly one INDEPENDENT variable; found 0.`

So the parameterised/shared-argument variant does **not** currently work either.

Verified with:

```sh
./a4 run models/test/ida/nested_array_independent.a4c \
  -m ida_nested_array_decay_param_t_build_failure \
  --integrate --duration 1 --steps 5 --no-test
```

## Exploratory patch result

I tested a local exploratory patch that did **not** get kept.

The patch changed independent-variable matching to treat clique-equivalent variables
as the same semantic variable, and changed the ODE validation to look through child
models instead of only the root link table.

That moved the `ARE_THE_SAME` case further forward, but it exposed a deeper issue:

- IDA then saw `n_y = 3`
- but only `n_ydot = 1`

This was observed with `gdb` in `solvers/ida/idaanalyse.c`.

At the same time, `system_get_diffvars(...)` reported:

- `nindep = 1`

So the independent-variable side can be made consistent, but the derivative side is
still wrong for arrayed child dynamic models.

Interpretation:

- the shallow "multiple INDEPENDENT declarations" handling is only the first symptom
- a deeper mismatch exists between:
  - hidden/materialised `der(...)` handling
  - dynamic-registry / diffvar assembly
  - IDA list construction

That is why I reverted the exploratory patch instead of keeping a partial fix.

## What we can say with confidence

- A single child dynamic model works today.
  See `ida_initial_hier_decay` in `models/test/ida/initial.a4c`.
- An array of child dynamic models using `der(...)` syntax does not currently work.
- Merging the child independents with `ARE_THE_SAME` does not currently make it work.
  This is already covered by an explicit negative regression and should stay that way.
- Passing one shared `t` into the child models as a model argument also does not
  currently make it work.
  This is now covered by an explicit negative regression too.

## Desired behaviour

For the non-merged case, the intended behaviour should be:

- fail the build
- report one clear user-facing error saying that multiple distinct `INDEPENDENT`
  variables were found across derivative chains
- explain that the child models must either share one independent variable
  or be restructured so that only one independent variable exists in the system

The intended behaviour should **not** be:

- raw internal `LINK`/`declarative link` chatter
- duplicate-LINK warnings that do not tell the user whether anything is wrong
- a follow-on `found 0` independent-variable message after we already detected
  multiple distinct independents

In short: for multiple non-merged `INDEPENDENT` instances, error out early,
once, and clearly.

## Likely problem areas

- `ascend/system/analyze.c`
  - `classify_instance`
  - current "same independent variable" check
- `ascend/system/system.c`
  - root-level `INDEPENDENT` validation
- `ascend/system/diffvars.c`
  - assembly of differential chains and independent-variable lists
- `ascend/compiler/derivinst.c`
  - hidden/materialised derivative handling for `der(...)`
- `solvers/ida/idaanalyse.c`
  - IDA variable/list construction, where the exploratory run showed `3` states but only `1` derivative variable

## Where the `INDEPENDENT` check should live

The canonical "exactly one system independent variable" check should most likely
run after instantiation, merging, and parameter passing have already resolved the
final instance graph.

Why:

- `INDEPENDENT t;` is lowered by the parser into a plain `LINK('independent', t)`.
- `DER(...)` statements are also lowered into LINK metadata (`LINK('ode', ...)`).
- Instantiation currently just executes those links and stores them.
- The actual dynamic meaning is only inferred later during system-build analysis.

That means parsing and raw LINK insertion are too early to decide whether two
independent variables are really distinct from the final model's point of view.

The authoritative check therefore belongs in system-build analysis on the fully
instantiated tree, after aliasing/clique merges and parameterised child instances
have had a chance to collapse onto the same semantic variable.

The later solver-specific code should then only be checking consistency with that
decision, not rediscovering independent-variable identity for itself.

## What `der(...)` currently associates with an independent variable

Current `der(...)` handling appears to be mostly independent-variable agnostic.

- `der(x)` is later rebound onto a materialised derivative variable for `x`.
- That binding step does not appear to carry an explicit "this derivative belongs
  to independent variable `t`" identity.
- The independent variable is instead tracked separately via the `independent`
  and `ode` link metadata.

There is one relevant exception:

- hidden derivative-instance dimension logic in `ascend/compiler/derivinst.c`
  scans for a unique independent variable dimension
- but this is about dimensions, not derivative-chain ownership

So the working contract seems to be:

- `der(...)` only says "this variable is dynamic"
- the final built system must then provide one coherent independent variable for
  all derivative chains

That matches the current evidence:

- the non-merged case fails because the system cannot reconcile multiple child
  independents into one final system independent variable
- but even when independent-variable identity is relaxed, the deeper bug is that
  arrayed child derivatives still collapse incorrectly from three states to one
  derivative variable

## Suggested next steps

1. Improve the diagnostics first.
   - Replace or demote the raw `declarative link` / duplicate-LINK messages for this path.
   - Make the non-merged case terminate with one clear error about multiple
     distinct independent variables.
   - Do not emit the contradictory later `found 0` message once the earlier
     condition has already been detected.

2. Investigate where the three child `FREE der(c[i].y)` requests collapse to one effective `ydot`.

## Update: corruption root cause and fix

The derivative-chain corruption turned out to be in `ascend/compiler/relation.c`,
not in the later diffvar/IDA assembly.

Specifically:

- `BindDerivativeTermsOnSide(...)` mutates shared token relations copy-on-write.
- For the first two arrayed child relations, `RelationRefCount(rel) > 1`, so the
  relation body is copied before replacing `der(...)`.
- The code then tried to decide whether to continue on the copied lhs or rhs with:
  `side = (side == RTOKEN(rel).lhs) ? RTOKEN(rel).lhs : RTOKEN(rel).rhs;`
- But after the copy, `RTOKEN(rel).lhs` is already the *new* lhs pointer, while
  `side` still points at the *old* lhs buffer.
- That comparison therefore fails, and the binder switches to the rhs by mistake.

Observed consequence:

- before copy, the `e_der` term in `c[1].dyn` and `c[2].dyn` correctly points to `y`
- after the mistaken side switch, derivative materialisation reads the rhs term
  instead and binds a hidden derivative for `k`
- `c[3].dyn` does not hit the shared-copy path, so it continues to bind correctly

The fix is narrow:

- record whether the current side is lhs/rhs *before* the copy
- reselect lhs/rhs using that saved boolean after the copy

After that fix:

- the `ARE_THE_SAME` case builds, analyses, and solves correctly
- the shared-parameter `t` case also builds, analyses, and solves correctly
- the unmerged case now fails cleanly with one explicit user-facing error about
  multiple distinct `INDEPENDENT` variables in the resolved system

So the current state is:

- multiple distinct child independents: reject clearly
- merged/shared child independents: supported and test-covered
   - This looks like the real structural bug once the independent-variable counting issue is relaxed.

## Working hypothesis

The current failures are not just "multiple uses of `INDEPENDENT` are illegal".
They look more like:

- independent-variable identity is handled inconsistently across the compiler/system/IDA layers
- and the `der(...)` materialisation path for arrayed child submodels is also incomplete

So we should treat this as a dynamic-structure consistency bug, not only as a front-end validation bug.

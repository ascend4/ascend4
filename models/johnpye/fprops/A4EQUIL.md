# A4EQUIL: Reactive Thermodynamics and Reactor Models for ASCEND

This note is an RFC-style design draft for adding chemically reactive
thermodynamics to ASCEND using the existing `fprops/eqm` Gibbs
minimization core.

It covers:

- a proposed `reactive_package` concept
- a new `reactive_state` / `reactive_stream` model family
- two first reactor models:
  - `reactor_equil`
  - `reactor_stoic`
- the black-box interfaces required to support them
- the governing matter and energy balances
- how the ASCEND equations map to the internal KKT system in FPROPS
- expected numerical issues, stability concerns, and derivative strategy

This is intentionally an end-to-end design note rather than a code-level
implementation plan for one file.

## 1. Why A New Stack Is Needed

ASCEND already has:

- legacy multicomponent/multiphase stream and thermo models in
  [../../thermodynamics.a4l](../../thermodynamics.a4l)
- legacy process unit models such as
  [../../stream_holdup.a4l](../../stream_holdup.a4l)
  and [../../reactor.a4l](../../reactor.a4l)
- a newer Gibbs-minimization equilibrium kernel in
  [eqm.h](eqm.h)
  and [eqm.c](eqm.c)

The current gap is architectural:

- the old A4 thermodynamics stack is built around fixed-species phase
  redistribution
- the new `fprops/eqm` code is built around elemental conservation and
  Gibbs minimization over candidate species/endmembers
- there is no ASCEND-facing integration of the new chemical equilibrium
  capability

The proposal in this note is to add a parallel `reactive_*` stack that
uses `fprops/eqm` as its thermodynamic core.

This avoids forcing chemical equilibrium into the old
`thermodynamics.a4l` formulation too early, while preserving a route to
future unification.

## 2. Guiding Design Principles

The proposed design is based on the following principles.

### 2.1 Use elements as the true conservation basis

Chemical reaction destroys any simple species-balance basis, but it does
not destroy elemental conservation. The natural conserved basis for
equilibrium and stoichiometric reactors is therefore:

- elemental amounts for closed systems
- elemental flow rates for open steady systems

This is a conceptual and thermodynamic statement first. It does not yet
force the implementation decision that every elemental balance must be
materialized explicitly as A4 equations in the first version.

### 2.2 Keep equilibrium species separate from stream species

Three different bases should be allowed to coexist:

- `elements`
- `internal equilibrium species`
- `external stream components`

For rigorous packages containing solution phases, internal equilibrium
species may include endmembers or site-species that are not appropriate
as user-facing process components.

However, the first rigorous implementation should be conservative:

- for packages involving solution phases, it is acceptable and probably
  preferable to set `external stream components = internal species`
- friendlier aggregations should initially be reporting aids, not the
  transport basis

This avoids irreversible information loss in connected flowsheets.

### 2.3 Keep the ASCEND instance tree light

The stream/state layer must not carry the full species database or all
equilibrium metadata in the instance tree.

Only the information needed to define model structure should be exposed
in A4:

- the stream component set
- package identity

Heavy thermodynamic detail should remain in a C-side runtime package
cache.

### 2.4 Keep the first deliverables narrow

The first rigorous user-facing units should be:

- `reactor_equil`: a single mixed-outlet equilibrium reactor
- `reactor_stoic`: a stoichiometric reactor using the same reactive
  thermodynamic package system

Phase-splitting units such as a reactive flash should come later.

This does not mean implementation work has to start with A4 model
wiring. A sensible bring-up order is:

1. implement and test a standalone equilibrium black-box function in C
2. expose that function to Python and small C test programs
3. only then wire it into ASCEND models

That sequence reduces risk while keeping the user-facing target
unchanged.

## 2A. Existing Reactor Library Context

Before proposing a new reactive stack, it is useful to record what the
existing reactor library already provides.

The current reactor library in
[../../reactor.a4l](../../reactor.a4l) is a
kinetics-based CSTR library, not a chemical-equilibrium library.

It provides two reactor models:

- `single_phase_cstr` in
  [../../reactor.a4l#L47](../../reactor.a4l#L47)
- `multiple_phase_cstr` in
  [../../reactor.a4l#L245](../../reactor.a4l#L245)

These models rely on the rate-based source-term interface from
[../../kinetics.a4l](../../kinetics.a4l), especially
`base_kinetics`, `element_kinetics`, and `specify_kinetics`.

What they support:

- steady and dynamic operation
- component holdup balances
- total mass and energy balances
- heat input `Qin`
- concentration-based reaction rates
- in the multiphase case, vapor-liquid thermodynamics using the old
  `thermodynamics.a4l` framework
- optional VLE coupling via `equilibrated := TRUE`

What they do not support:

- elemental-basis reaction closure
- Gibbs minimization
- internal equilibrium species distinct from stream species
- stoichiometric reactor closure in the `RSTOIC` sense
- chemical equilibrium in the `RGibbs` sense

The most relevant historical precedent for this RFC is the multiphase
reactor demonstration in
[../../reactor.a4s#L46](../../reactor.a4s#L46), where
`test_multiple_phase_cstr` is rerun with `equilibrated := TRUE`.

That example is useful because it already demonstrates one important
architectural pattern:

- reactor balances in A4
- thermodynamic closure supplied by a state submodel
- steady and dynamic modes supported by different `seqmod` choices

However, the chemistry in that library remains kinetic-source-based. The
new `reactor_equil` proposed here should therefore be seen as a new
reactive-thermodynamic unit family that reuses some modeling patterns
from `reactor.a4l`, but not its chemical closure.

## 3. Terminology and Bases

The terms below are used throughout this note.

### 3.1 Elements

Conserved basis variables:

- closed system: `b[e]`
- steady open system: `b_dot[e]`

Examples:

- `Fe`
- `O`
- `H`
- later `C`, `N`, `S`, charge, site constraints, etc.

### 3.2 Internal equilibrium species

These are the species/endmembers over which the Gibbs minimization runs.

For Fe-O-H a possible internal basis is:

- `Fe_bcc`
- `Fe_fcc`
- `Fe2O3`
- `Wus_FeO`
- `Wus_FeO1p5`
- `Sp_Fe2_tet`
- `Sp_Fe3_tet`
- `Sp_Fe2_oct`
- `Sp_Fe3_oct`
- `Sp_Va_oct`
- `hydrogen`
- `water`

The point is not that this is the only basis, but that the equilibrium
kernel must be free to work on a basis more detailed than the visible
stream basis.

### 3.3 External stream components

These are the components carried on ports and shared between connected
units. They define stream dimensionality in ASCEND.

For simple reactive packages they may coincide with internal species.
For more user-friendly or more approximate packages they may be a
coarser basis.

For the first delivered version, they should normally coincide with the
internal equilibrium species basis. That keeps the first wiring exact
and avoids premature work on pseudo-component transport.

### 3.4 Report groups and GUI adaptation

These are named linear aggregations of internal species that may later
be useful for reporting or GUI display:

`n_report[g] = SUM[R[g,i] * n_internal[i]]`

Examples:

- `metal_iron = Fe_bcc + Fe_fcc`
- `wustite_total = Wus_FeO + Wus_FeO1p5`
- `spinel_total = Sp_Fe2_tet + Sp_Fe3_tet + Sp_Fe2_oct + Sp_Fe3_oct + Sp_Va_oct`

They are not initially proposed as the transport basis for streams.
They are also not required for the first implementation milestone.

### 3.5 Nonlinear descriptors

Some useful reported quantities are not linear sums and therefore are
not report groups. Examples:

- `x_wus = Wus_FeO1p5 / (Wus_FeO + Wus_FeO1p5)`
- spinel site fractions
- phase fractions

These should be treated as derived outputs, not part of the primary
stream basis. They are also secondary to the first implementation goal
of getting rigorous models wired and solved.

## 4. Proposed `reactive_package`

## 4.1 Role

The `reactive_package` defines the chemical and thermodynamic universe
for a set of connected reactive units.

It answers questions such as:

- what stream components exist?
- what internal equilibrium species exist?
- which thermo models and source maps are used?
- how are stream components mapped to elements?
- later, how should internal states be presented to users?

The package is the object that should be shared with
`WILL_BE_THE_SAME`-style constraints across a reactive flowpath.

For a first implementation, the likely user experience is:

- the A4 model names or instantiates a package identity
- that package identity selects a detailed runtime package definition
  held in C-side code or loaded through C-side package infrastructure

In other words, users should initially select packages, not assemble all
species/source-map detail manually inside each process model.

## 4.2 Hybrid A4/C design

The package should exist in two forms.

### ASCEND-side package object

This is intentionally light. It should expose only what ASCEND needs to
instantiate arrays and validate connections:

- `stream_components`
- `package_name`
- `package_version`
- `package_key`

The `package_key` should be a stable identity or hash, not a raw pointer.

For the first design pass, there is no strong reason to expose the
element set itself in the A4 instance tree. The black-box code can own
the element basis completely.

### Why this matters for usability

The package boundary is not only an implementation concern. It affects:

- how easy it is for a user to write a model
- how much clutter appears in the instance tree
- how much debug information is visible from ASCEND
- how easy it is to reproduce and audit a calculation

The first package design should therefore prefer:

- simple, explicit package identity in A4
- rich runtime introspection in C/Python
- minimal repeated thermo metadata in process models

The user should not be forced to rebuild large thermo package
definitions ad hoc inside every reactor model.

An initial practical direction is:

- A4 package object carries a name/key and stream component set
- C runtime registry maps that key to the full thermo package

That keeps A4 models writable while still allowing sophisticated
runtime package definitions.

### C-side runtime package

This is the heavy representation built and cached behind the package key.
It contains:

- internal equilibrium species
- elemental incidence matrices
- thermo source maps
- phase model definitions
- solution-phase metadata
- later, report groups
- later, nonlinear descriptor definitions
- GUI/reporting metadata

This package can be cached and reused by all black-box relations using
the same key.

## 4.3 Why not keep everything in the instance tree?

Because every reactive stream would otherwise carry:

- large sets and arrays for internal species
- source-map strings
- phase-model metadata
- report matrices

That would scale poorly in flowsheets with many streams and units.

## 4.4 Why not make the package fully opaque?

Because ASCEND still needs to know the stream component set at
instantiation time. If a stream has:

```ascend
f[components] IS_A molar_rate;
```

then `components` must be visible in A4. Structure cannot be discovered
at solve time.

The package therefore needs to be light, but not completely opaque.

## 5. Proposed Stream and State Redesign

The recommendation is to add a new family rather than rewriting the
legacy `stream`/`thermodynamics` stack immediately.

Proposed first-generation models:

- `reactive_state`
- `reactive_stream`
- `reactive_holdup`

The old models continue to exist unchanged.

## 5.1 `reactive_state`

This is the state model for a reactive mixture at a specified
composition basis.

At minimum it should include:

- `pkg WILL_BE reactive_package`
- `components ALIASES pkg.stream_components`
- `P`
- `T`
- `y[components]`
- `H`
- `V`
- `phase_summary` outputs optionally later

In the first implementation, `reactive_state` should not try to encode
all internal equilibrium detail as A4 arrays. Instead, its thermodynamic
properties are supplied by black-box relations using the package key.

`V` should not be treated as optional for the long-term design because
reactor sizing and residence-time calculations depend on it. It may be
staged later in implementation if needed, but it belongs in the model
conceptually.

## 5.2 `reactive_stream`

This parallels `detailed_stream` in
[../../stream_holdup.a4l](../../stream_holdup.a4l).

At minimum:

- `state WILL_BE reactive_state`
- `flow`
- `f[components]`
- `H_flow`
- `P`, `T` aliased to `state.P`, `state.T`

The familiar stream relation still applies:

`f[k] = y[k] * flow`

but total molar flow is no longer conserved across a reactor.

## 5.3 Package compatibility across a flowpath

Connected reactive units should share the same package object, or at
least the same package key. Conceptually this is similar to:

```ascend
inlet.pkg, outlet.pkg WILL_BE_THE_SAME;
```

This does not mean that every stream must expose every internal
equilibrium species unless the package chooses that as its stream basis.
It means that all connected units agree on the same transport basis and
runtime package semantics.

## 6. Mapping Between External and Internal Bases

This is the central conceptual issue.

## 6.1 Input mapping: external components to elements

For the inlet side, no explicit external-to-internal species map is
required.

Instead, the package provides an external component elemental matrix:

`A_stream[e,k]`

Then:

`b_dot[e] = SUM[A_stream[e,k] * f_in[k]]`

for steady open systems, or

`b[e] = SUM[A_stream[e,k] * n[k]]`

for closed systems.

This is robust because the equilibrium kernel only requires elemental
totals.

In implementation terms there are two reasonable choices:

- compute `b` explicitly in ASCEND and pass it to the black-box
- pass component flows to the black-box and let C compute `b` internally

The thermodynamic primitive should still be treated as `TPb`, because
that is the natural reusable kernel. However, for the first ASCEND
integration it may be cleaner to let the C layer compute `b` internally
from stream component flows so that the A4 models stay smaller.

## 6.2 Internal equilibrium solve

The runtime package provides an internal species elemental matrix:

`A_internal[e,i]`

The FPROPS kernel then solves over `n_internal[i]` or `f_internal[i]`.

`A_internal` belongs in the runtime package in C, not in the A4
instance tree.

## 6.3 Output mapping: internal basis to stream basis

This is harder because information can be lost.

If internal basis contains:

- `Wus_FeO`
- `Wus_FeO1p5`

and the external stream basis contains only:

- `wustite`

then the stream loses the wustite composition state unless an auxiliary
descriptor is also transported.

For that reason, the first rigorous implementation should be cautious.

### Recommended first rule

For packages with solution phases or endmember-level detail:

- set `stream_components = internal equilibrium species`
- postpone report groups and descriptors to GUI/introspection work

This preserves information exactly and keeps the first implementation
focused on solvability.

### Later extension

Once there is a robust notion of package-specific pseudo-components plus
auxiliary descriptors, a coarser stream basis may be introduced.

## 7. Black-Box Function Strategy

The existing `fprops` integration in
[asc_fprops.c](asc_fprops.c)
provides the pattern:

- ASCEND-side black-box relations with `INPUT`, `OUTPUT`, `DATA`
- C-side package lookup and cached runtime evaluation

Reactive thermodynamics should follow this same pattern.

## 7.1 Separate state closure from unit model

The core thermodynamic closure should be independent of reactor type:

`state = Phi(pkg, T, P, extensive elemental basis)`

Different units then wrap this closure:

- equilibrium reactor
- batch equilibrium vessel
- equilibrium flash
- dynamic CSTR
- later PFR slices or collocation sections

## 7.2 Proposed first black-boxes

### `fprops_eqm_tpb`

Purpose:

- equilibrium composition and enthalpy at specified `T`, `P`, and
  elemental totals/flows

Inputs:

- `T`
- `P`
- `b[e]` or `b_dot[e]`

Outputs:

- `n_eq[k]` or `f_eq[k]` in stream basis
- `H_eq` or `Hdot_eq`
- solver status / diagnostics in standalone C and Python interfaces
- later, optional phase summary outputs

Data:

- `reactive_package`

Internally this black-box:

1. converts package key to runtime package
2. builds/uses `A_internal`
3. calls `eqm_solve_elements(...)` or a package-aware extension
4. evaluates equilibrium enthalpy from the resulting equilibrium state
5. maps outputs to stream basis

This should be the first concrete implementation target. It is useful
even before any ASCEND model wiring because it can support:

- C-level regression tests
- Python experiments
- CLI-style utilities
- later ASCEND black-box bindings

### Failure behavior

`fprops_eqm_tpb` must not silently return non-conservative or partially
updated results on a failed solve. On failure it should:

- return a clear non-success status
- preserve diagnostic information useful for debugging
- avoid presenting the caller with an apparently valid equilibrium state

Element conservation should therefore be guaranteed for successful
returns and treated as invalid/undefined for failed returns.

### `fprops_rxnprops_tpf`

Purpose:

- thermodynamic properties of a specified reactive mixture at `T`, `P`
  and stream composition

Inputs:

- `T`
- `P`
- `f[k]` or `y[k]`

Outputs:

- `H`
- later `V`, `G`, phase summary, etc.

Data:

- `reactive_package`

This is not an equilibrium solver. It is a property evaluator for a
given reactive-package composition basis. It is mainly needed by
`reactor_stoic` and by generic reactive stream states.

It is probably straightforward once `fprops_eqm_tpb` exists, but it is
not the primary immediate target.

## 7.3 Derivative support

If reactive units are to solve robustly inside ASCEND, these black-boxes
need derivative callbacks, not only residual evaluation.

For `fprops_rxnprops_tpf`, standard black-box derivative support should be
added directly if the property model can supply it.

For `fprops_eqm_tpb`, derivatives are more subtle because the equilibrium
state is itself the result of an optimization/KKT solve. The correct
approach is implicit differentiation of the equilibrium conditions, not
naive finite differencing in the long term.

Here "implicit differentiation" means this:

- treat the converged equilibrium state as satisfying a nonlinear system
  of KKT equations
- differentiate that KKT system with respect to `T`, `P`, or `b`
- solve the resulting linear system for the sensitivities of the
  equilibrium state

So the sensitivity calculation is not "rerun equilibrium many times with
small perturbations". It is "differentiate the already-satisfied KKT
system and solve for `dn/dT`, `dn/dP`, `dn/db`".

## 8. `reactor_equil`

## 8.1 Role

`reactor_equil` is the first Gibbs-equilibrium process unit.

It is conceptually similar to an `RGibbs` reactor in other simulators,
but expressed in ASCEND's equation-based style.

The first version should be:

- steady-state
- single mixed outlet
- no phase separation
- no explicit residence-time model

This is not specifically a CSTR, batch reactor, or PFR. It is a
single-control-volume steady equilibrium reactor.

## 8.2 Minimum structure

Suggested model parts:

- `inlet WILL_BE reactive_stream`
- `outlet WILL_BE reactive_stream`
- `pkg ALIASES inlet.state.pkg`
- `Qin IS_A energy_rate`
- `dP` or `Pdrop`

Compatibility constraints:

- `inlet.state.pkg, outlet.state.pkg WILL_BE_THE_SAME`

Primary variables:

- outlet `T`
- outlet `P`
- outlet species flows `outlet.f[components]`
- `Qin` unless adiabatic

There should be no `equilibrated` switch on `reactive_stream` or
`reactive_state`. In the new stack, equilibrium should be a property of
the unit model or closure being used, not a toggle on the stream object.

## 8.3 Governing balances

For a single-inlet, single-outlet, steady reactor with no shaft work:

### Balance basis

Conceptually, the reactor is governed by:

- elemental conservation
- energy conservation
- pressure relation
- equilibrium closure

In a fully explicit formulation, elemental balances would appear in A4.
However, for the first implementation it may be better to treat them as
internal to the `fprops_eqm_tpb` closure:

- ASCEND supplies inlet component flows
- C computes inlet element totals
- C solves equilibrium at outlet `T`, `P`
- C returns outlet composition and enthalpy consistent with those same
  element totals

That still makes the model element-based thermodynamically, even if the
first A4 model does not materialize a visible `b[e]` array.

### Pressure relation

For example:

`outlet.P = inlet.P - dP`

### Equilibrium closure

At the outlet state:

Conceptually:

`(outlet.f[components], H_out, aux) = fprops_eqm_tpb(outlet.T, outlet.P, b_dot_in, pkg)`

In practice the black-box may be written as a set of relations equating
ASCEND variables to the returned equilibrium outputs. If the first A4
binding passes inlet component flows rather than an explicit `b_dot_in`
array, that should be treated as a wrapper around the same `TPb`
primitive, not a different thermodynamic kernel.

### Energy balance

`outlet.H_flow = inlet.H_flow + Qin`

or equivalently:

`outlet.flow * outlet.state.H = inlet.flow * inlet.state.H + Qin`

For adiabatic operation:

- fix `Qin = 0`
- solve for outlet `T`

For isothermal operation:

- fix outlet `T`
- solve for `Qin`

This is why the equilibrium black-box should return absolute enthalpy,
not only `delta H_rxn`.

## 8.4 Why total molar flow should not be separately conserved

Total molar flow generally changes under reaction. The correct conserved
quantity is elemental flow. Therefore:

- do not impose `inlet.flow = outlet.flow`
- let outlet total flow emerge from equilibrium outlet species flows

## 9. `reactor_stoic`

## 9.1 Role

`reactor_stoic` is the simpler stoichiometric reactor, analogous to
`RSTOIC`-style models in other simulators.

It uses:

- a user-defined reaction set
- user-defined extent, conversion, or conversion basis
- the same reactive package/property framework as `reactor_equil`

It does not perform Gibbs minimization.

## 9.2 Why include it in the same RFC?

Because:

- it shares the same package and stream design
- it shares the same reactive property evaluation
- it is likely to be useful earlier to users than the full equilibrium
  reactor in some cases
- it provides a lower-risk adoption path

## 9.3 Suggested model structure

- `inlet WILL_BE reactive_stream`
- `outlet WILL_BE reactive_stream`
- `rxnset WILL_BE stoic_reaction_package` or a simpler reaction-data object
- `extent[r]` or `conversion` variables
- `Qin`

Species balances in stream basis:

`outlet.f[k] = inlet.f[k] + SUM[nu[k,r] * extent[r]]`

where `nu[k,r]` is the stoichiometric coefficient in the stream basis.

If the chosen stream basis is not compatible with the user's desired
reaction specification, then the package or reaction set must define the
appropriate mapping.

## 9.4 Thermodynamic closure

`reactor_stoic` still needs reactive-package property evaluation:

`outlet.state.H = fprops_rxnprops_tpf(outlet.T, outlet.P, outlet.f[components], pkg)`

Energy balance is then written exactly as for the equilibrium reactor.

## 9.5 Stoichiometric reaction basis

Unlike equilibrium, stoichiometric reactions do depend on a chosen
reaction basis. That is acceptable here because it is a user-specified
reactor model, not a universal thermodynamic closure.

## 9.6 `reactor_kinetic`

## 9.6.1 Role

`reactor_kinetic` is the rate-based reactor family.

It should cover:

- conventional kinetic CSTR models
- staged kinetic reactor models
- future PFR/distributed models that reuse the same rate-law basis

It should **not** enforce equilibrium closure. It is the direct
successor to the older kinetics-only ideas in
[reactor.a4l](/home/john/ascend/models/reactor.a4l), but rebuilt on the
new reactive package and stream basis.

## 9.6.2 Minimum contract

Suggested model parts:

- `inlet WILL_BE reactive_stream`
- `outlet WILL_BE reactive_stream`
- `pkg ALIASES inlet.state.pkg`
- `rxn WILL_BE kinetic_reaction_set`
- `V IS_A volume`
- `Qin IS_A energy_rate`
- `dP` or `Pdrop`

Compatibility constraints:

- `inlet.state.pkg, outlet.state.pkg WILL_BE_THE_SAME`
- kinetic stoichiometry must be defined on the same stream/package basis

Primary variables:

- outlet `T`
- outlet `P`
- outlet species flows `outlet.f[components]`
- reactor size / holdup variable such as `V`
- any independent rate variables if exposed
- `Qin` unless adiabatic

## 9.6.3 Governing equations

For steady-state CSTR-style operation, the natural basis is:

`outlet.f[k] = inlet.f[k] + V * SUM[nu[k,r] * rate[r] | r IN rxns]`

with:

- `nu[k,r]` = stoichiometric coefficient on the package species basis
- `rate[r]` = molar production rate per reactor volume

Energy balance:

`outlet.H_flow = inlet.H_flow + Qin`

Pressure relation:

`outlet.P = inlet.P - dP`

This is deliberately parallel to `reactor_stoic` and `reactor_equil`.
The main difference is that extent/equilibrium closure is replaced by a
rate-law closure.

## 9.6.4 Thermodynamic support needed

`reactor_kinetic` still needs package-backed thermodynamics:

- outlet enthalpy for energy balance
- concentrations, mole fractions, or activities for rate laws
- later density / molar volume for residence-time and hydrodynamic work

The first implemented version uses a `reactive_holdup` object and:

- `fprops_rxn_h_TPn(...)` for stream enthalpy
- `fprops_rxn_v_TPn(...)` for best-available package molar volume
- direct A4 expressions for holdup concentrations

This first `reactive_holdup.V` should be interpreted as an engineering
holdup volume, not yet as a universally thermodynamically consistent
mixture volume. At present:

- `helmholtz` and `pengrob` backed species can provide volume directly
- `constcp` and `shomate` species can provide approximate condensed
  molar volume from density metadata
- solution phases such as wustite/spinel do not yet provide volume here

So the first `reactor_kinetic` path is intentionally limited to package
bases where engineering molar volume is available.

Later versions will likely want additional package property calls for:

- phase-specific activities
- mixture density / molar volume
- equilibrium constants or reaction Gibbs energies

## 9.6.5 Why keep `reactor_kinetic` separate

This separation is useful because many real process models need:

- explicit catalyst kinetics
- mass-transfer-limited kinetics
- shrinking-core or morphology corrections
- user-prescribed empirical rate laws

without any requirement that the model be driven toward equilibrium.

## 9.6.6 First delivered shape

The first practical target is a steady single-phase CSTR aligned with the
older `test_single_phase_cstr` example in
[../../reactor.a4l](../../reactor.a4l):

- same reversible hydrocarbon chemistry
- same isothermal closure
- same kinetic law family via `element_kinetics`
- new `reactive_stream` / `reactive_holdup` / FPROPS thermo path

That makes the first verification target "same chemistry and same
reactor closure as the legacy model", while keeping the new work focused
on the reactive-package thermo layer rather than rewriting the old
kinetics equations unnecessarily.

## 9.7 `reactor_kineq`

## 9.7.1 Role

`reactor_kineq` is the kinetics-toward-equilibrium reactor family.

It sits between:

- `reactor_kinetic`: kinetics with no equilibrium target
- `reactor_equil`: instantaneous equilibrium with no kinetics

This is likely to be especially important for iron-reduction and similar
flowsheets, where reaction rates are finite but the equilibrium limit
still matters strongly.

## 9.7.2 Core idea

The defining feature of `reactor_kineq` is:

- rates are finite
- rates depend on thermodynamic driving force
- rates go to zero at equilibrium

So equilibrium is **not** imposed as a separate black-box composition
solve inside the reactor. Instead, equilibrium information appears
inside the rate law.

Typical forms include:

`rate[r] = rate_fwd[r] * (1 - Q[r] / K[r])`

or:

`rate[r] = rate_fwd[r] - rate_rev[r]`

with:

`K[r] = exp(-DeltaG0[r] / (R * T))`

and `Q[r]` formed from the same activity basis as the kinetics model.

## 9.7.3 Minimum contract

Suggested model parts:

- `inlet WILL_BE reactive_stream`
- `outlet WILL_BE reactive_stream`
- `pkg ALIASES inlet.state.pkg`
- `rxn WILL_BE kineq_reaction_set`
- `V IS_A volume`
- `Qin IS_A energy_rate`
- `dP` or `Pdrop`

Compatibility constraints:

- `inlet.state.pkg, outlet.state.pkg WILL_BE_THE_SAME`
- kinetic/equilibrium stoichiometry must be defined on the same package
  basis
- the activity basis used in `Q[r]` must be consistent with the package
  thermodynamic model

Primary variables:

- outlet `T`
- outlet `P`
- outlet species flows `outlet.f[components]`
- `rate[r]`
- `Qrxn[r]` or equivalent reaction quotient variables if exposed
- `K[r]` or `DeltaG0[r]` if exposed
- `V`
- `Qin` unless adiabatic

## 9.7.4 Governing equations

Species balances:

`outlet.f[k] = inlet.f[k] + V * SUM[nu[k,r] * rate[r] | r IN rxns]`

Energy balance:

`outlet.H_flow = inlet.H_flow + Qin`

Pressure relation:

`outlet.P = inlet.P - dP`

Thermodynamic driving-force relation for each reaction:

`Qrxn[r] = PROD[a[k] ^ nu[k,r] | participating species]`

`K[r] = exp(-DeltaG0[r] / (R * outlet.T))`

Rate-law closure, for example:

`rate[r] = kf[r] * driving[r] * (1 - Qrxn[r] / K[r])`

where `driving[r]` may include:

- reactant concentration/activity factors
- catalyst effectiveness
- gas-solid contact terms
- shrinking-core or surface-area terms

The exact rate form should remain open and belong in a rate-package
object, not hard-coded into the reactor shell.

## 9.7.5 Required thermodynamic support

`reactor_kineq` needs more than `reactor_kinetic`:

- stream enthalpy for energy balance
- composition/activity variables for `Q[r]`
- standard reaction Gibbs energy or equilibrium constant `K[r]`

That suggests a later property helper family such as:

- `fprops_rxn_h_TPn(...)`
- `fprops_rxn_delta_g0_tp(...)` or equivalent
- possibly direct activity / fugacity helper functions for selected
  package types

The first implementation does **not** need a full equilibrium solve
inside the reactor. It only needs enough thermodynamics to compute the
equilibrium target used in the rate law.

## 9.7.6 Why this is likely important for iron reduction

For iron-reduction flowsheets, the physically important regime is often:

- not fully equilibrium-limited
- not purely empirical kinetics either
- strongly influenced by gas composition relative to equilibrium

So `reactor_kineq` is a natural building block for:

- staged shaft-furnace models
- gas-solid reduction trains
- models where `H2/H2O` or `CO/CO2` ratios control approach to
  reduction limits

It should therefore be treated as a first-class future reactor family,
not a minor variation on `reactor_kinetic`.

## 9.7.7 Current implementation status

The first implementation path should stay narrow.

What is implemented now:

- a first `reactor_kineq` shell on the same `reactive_stream` /
  `reactive_holdup` basis as `reactor_kinetic`
- a first `kineq_reaction_set` using a concentration-based
  forward-minus-reverse elementary rate form:

  `rate[r] = rate_fwd[r] - rate_rev[r]`

  with:

  `rate_rev[r] = (k_fwd[r] / K_eq[r]) * exp(-Ea[r]/RT) * PROD[(c_i/c_ref)^nu+ ]`

  where `Qc[r]` is formed from concentrations normalized by a fixed
  reference concentration

What this current form is good for:

- bringing up the reactor shell
- matching reversible elementary kinetics when `K_eq` is supplied on the
  same concentration basis
- comparing against the legacy `reactor.a4l` single-phase CSTR behavior

Current limitation:

- the first `reactor_kineq` formulation using the compact
  `rate_fwd * (1 - Q/K)` form did not converge robustly with `QRSlv`
- the forward-minus-reverse reformulation does converge for the first
  single-phase reversible-CSTR case

Important implementation note:

- `rate[r]` and `production[i]` must be allowed to go negative; using the
  default nonnegative `conc_rate` bounds causes false solve failures

Current best initialization path:

- solve the matching `reactor_kinetic` case first
- copy the solved stream / holdup state into `reactor_kineq`
- then solve `reactor_kineq` from that initialized state

This continuation path is now implemented as a regression harness and
serves as the cleanest verification that `reactor_kineq` reproduces the
reversible kinetic baseline when both are posed on the same
concentration basis.

So the practical near-term plan is:

1. keep `reactor_kineq` v1 concentration-based
2. verify `reactor_kinetic` directly against the legacy reversible CSTR
3. use that comparison as the anchor for later `reactor_kineq`
   continuation
4. only after robust solve behavior is achieved, upgrade `reactor_kineq`
   from concentration-based `Qc/K_eq` toward a package-consistent
   activity / fugacity basis

## 10. Governing Thermodynamic Basis Inside FPROPS

The equilibrium kernel in
[eqm.c](eqm.c)
already solves the right class of problem:

minimize total Gibbs free energy over species amounts, subject to
element conservation and non-negativity.

In abstract form:

`minimize G(T, P, n)`

subject to:

- `A n = b`
- `n_i >= 0`
- additional internal phase constraints where required

This already covers:

- pure condensed phases
- gas species
- binary condensed solutions
- Fe-spinel internal constraints

as implemented in the current `eqm` code.

## 10.1 KKT form

At equilibrium, the first-order optimality conditions are the KKT
conditions:

- stationarity:
  `mu + A^T lambda - s = 0`
- primal feasibility:
  `A n = b`
- non-negativity:
  `n >= 0`
- dual feasibility:
  `s >= 0`
- complementarity:
  `n_i * s_i = 0`

where:

- `mu = dG/dn`
- `lambda` are element potentials
- `s` are bound multipliers for `n_i >= 0`

These KKT conditions are the true thermodynamic closure being embedded in
ASCEND, even if the first implementation hides them inside a black-box.

It is conceivable that a future ASCEND integration could expose some or
all of this KKT system more directly to the outer solver. That could be
useful if there is later value in solving larger flowsheets and
equilibrium conditions in one more tightly orchestrated nonlinear
system.

However, that is explicitly not the first implementation target. The
first target is a robust, tested black-box equilibrium kernel.

## 10.2 Relation to old phase-equilibrium thinking

For nonreactive phase equilibrium, it is common to say "equal chemical
potential across phases". For chemically reactive equilibrium, the more
general and more useful formulation is Gibbs minimization under elemental
constraints.

The KKT system then implies the appropriate equalities of chemical
potential for active phases/species.

## 11. Matter and Energy Balance Basis

The design should be explicit about which balance basis is primary in
which model.

## 11.1 Equilibrium reactor

Primary balance basis:

- element flow rates
- total energy flow

Not primary:

- species balances
- total molar flow balance

Species flows are outputs of the equilibrium closure.

Implementation note:

- first version may keep the elemental balances implicit inside the
  equilibrium black-box
- later versions may choose to expose explicit `b[e]` arrays in A4 if
  that proves useful for transparency, diagnostics, or solver control

## 11.2 Stoichiometric reactor

Primary balance basis:

- species balances with stoichiometric source terms
- total energy flow

Element balances are implied by correct stoichiometry and may be used as
validation equations or diagnostics.

## 11.3 Closed systems and dynamic holdups

The same thermodynamic closure can also be used for:

- batch equilibrium vessels
- dynamic CSTR-like holdups

In those cases the extensive conserved basis is elemental holdup rather
than elemental flow.

## 12. Numerical Issues and Stability

Embedding equilibrium in an equation-oriented simulator raises numerical
issues that should be designed for from the beginning.

## 12.1 Non-negativity and active sets

The equilibrium solution naturally contains phase/species appearance and
disappearance. This means:

- active sets can change abruptly
- some equilibrium species can sit at very small positive floors
- derivatives can change structure when species enter or leave

The current `eqm` code already contains:

- reduced-space logic
- active-set seeding
- full-space pathways for solution phases

This should be preserved and not reimplemented in A4.

## 12.2 Singularities near zero amounts

Gas-mixture terms contain logarithms of composition, and solution-phase
models often contain logarithms of endmember fractions. Therefore:

- zero or negative amounts are forbidden in the internal solve
- small interior floors are required
- reported values should be robust to trace species

The black-box layer should never expose exact zero values internally to
the Newton iteration without care.

## 12.3 Scaling

Element totals and species amounts can vary by many orders of magnitude.
The current `eqm` code already applies scaling of elemental balances and
state variables. The ASCEND-facing layer should reinforce this by:

- using consistent engineering scales for `b[e]` or `b_dot[e]`
- setting good nominals on flow and enthalpy variables
- avoiding redundant equations

ASCEND's own scaling and FPROPS' internal scaling should be allowed to
coexist, but they live at different layers:

- ASCEND scaling controls the outer nonlinear solve
- FPROPS scaling stabilizes the inner equilibrium computation

The main requirement is that derivative callbacks presented to ASCEND
must correspond to the physical, unscaled variables at the interface.
Internal scaling should remain invisible outside the black-box.

## 12.4 Reference-state consistency

This is a thermodynamic rather than numerical issue, but it affects
solvability and trust in results.

Mixed-source packages must enforce reference-state consistency across:

- ideal gas species
- Helmholtz fluids
- Peng-Robinson fluids
- const-`cp` condensed species
- Shomate species
- condensed Gibbs-species data

This is already a central concern in the current `eqm` and Fe-O-H work
and must remain explicit in package validation.

## 12.5 Smoothness of output mapping

If the stream basis is coarser than the internal basis, output mapping
may involve:

- linear aggregation
- nonlinear descriptors
- discrete phase-presence interpretation

The first implementation should avoid discrete output logic in the
equation system. Report booleans such as "phase present" should be
diagnostic outputs, not solver variables.

## 12.6 Multiple local minima and metastability

The intended thermodynamic model is global equilibrium. In practice:

- some formulations may admit poor initial guesses
- some phase-model choices may represent only a restricted subset of
  the full chemistry
- metastable solutions may be of engineering interest later

The first implementation should target stable equilibrium only and make
metastability a later extension, if at all.

## 13. Derivatives, Sensitivities, and Hessians

This area is critical for adoption in ASCEND.

## 13.1 Why finite differences are not enough long-term

A black-box equilibrium relation without reliable derivatives will:

- converge slowly
- be sensitive to perturbation size
- struggle badly near active-set transitions
- limit usefulness in larger flowsheets

Finite differences may be acceptable only for very early bring-up.

## 13.2 Desired first derivatives

For `fprops_eqm_tpb`, the important sensitivities are:

- `d f_eq / dT`
- `d f_eq / dP`
- `d f_eq / db`
- `d H_eq / dT`
- `d H_eq / dP`
- `d H_eq / db`

These should be obtained by linearizing the equilibrium KKT system and
solving the resulting sensitivity system.

These calculations should not be massively costly relative to the
equilibrium solve if implemented carefully, because the same KKT
structure or factorization can often be reused for multiple right-hand
sides. They are still nontrivial, but they are much more attractive than
repeated finite-difference re-solves.

## 13.3 KKT linearization

If the equilibrium solve is represented abstractly by:

- stationarity equations
- elemental constraints
- active inequality set

then sensitivities follow from the linearized KKT matrix. This is the
correct route for robust derivative callbacks in the black-box API.

## 13.4 Second derivatives and Hessians

Full second derivatives may eventually be useful, but they are not
required for the first adoption target.

Likely difficulties:

- complementarity/active-set changes create nonsmoothness
- solution-phase models require reliable second derivatives internally
- aggregated outputs may add extra nonlinear layers

Therefore the recommended order is:

1. residual evaluation
2. first-derivative callbacks
3. only later consider Hessian callbacks

## 14. First-Phase Implementation Scope

The proposal for a first usable milestone is:

### 14.1 Immediate kernel work

- implement `fprops_eqm_tpb`
- expose it in C test code
- expose it in Python for rapid exploration and regression testing

This is already useful outside ASCEND and should be the first practical
implementation step.

### 14.2 New package and stream family

- add `reactive_package`
- add `reactive_state`
- add `reactive_stream`

### 14.3 Black-boxes

- bind `fprops_eqm_tpb` into ASCEND black-box form
- later implement `fprops_rxnprops_tpf`

### 14.4 First reactor models

- implement `reactor_equil`
- implement `reactor_stoic`

### 14.5 Initial package discipline

- for rigorous packages with solution phases, set
  `stream_components = internal equilibrium species`

### 14.6 Leave for later

- reactive flash / phase-splitting unit
- pseudo-component transport with auxiliary descriptors
- dynamic reactive holdups
- PFR discretizations
- metastable constraints

## 14.7 Proposed implementation checklist

To regain momentum while still protecting the longer-term architecture,
the next implementation steps should be:

1. implement `reactor_stoic`
2. extend `fprops_mix_h_tpn` only for the Fe-O-H species and phase
   models needed by `reactor_stoic` and `reactor_equil`
3. validate fast gas-phase equilibrium cases such as water-gas shift
4. validate Fe-O-H equilibrium at fixed `T`, `P`
5. implement `reactor_equil`
6. only then revisit general multiphase inlet wrappers, reactive flash,
   and nonideal liquid mixture enthalpy

This sequence is deliberate:

- `reactor_stoic` settles the package/stream interface, duty sign
  conventions, and energy-balance basis without simultaneously
  debugging Gibbs minimization
- a narrowly targeted enthalpy capability is enough to support the
  first reactor models
- faster gas-only regression cases keep development cycles short while
  Fe-O-H remains the main application driver
- `reactor_equil` should be added only after the species basis and
  energy basis are already proven in a simpler reactor

### 14.7.1 Proposed first file/model surface

The first implementation should be split into a small number of new A4
artifacts with clear roles.

Proposed A4 additions:

- `reactive_package`:
  compile-time package identity, stream component set, and opaque link
  to the C-side runtime package
- `reactive_state`:
  state variables and property hooks on one canonical package species
  basis
- `reactive_stream`:
  stream flow variables and a `state` part, analogous in role to the
  existing `stream` family but without the old `equilibrated` switch
- `stoic_reaction_set`:
  reaction count, stoichiometric coefficients, and optional reaction
  metadata such as reaction names and conversion bases
- `reactor_stoic`:
  inlet/outlet ports, pressure relation, heat duty, extent variables,
  and energy balance

The first practical split in the codebase should be:

- one A4 file for package/stream/state definitions
- one A4 file for stoichiometric reaction and reactor models
- one A4 test/demo file for regression examples

This keeps the package/stream design reusable by both
`reactor_stoic` and `reactor_equil`.

### 14.7.2 First `reactor_stoic` A4 contract

The first version of `reactor_stoic` should be deliberately simple.

Required parts:

- `pkg WILL_BE reactive_package`
- `inlet WILL_BE reactive_stream`
- `outlet WILL_BE reactive_stream`
- `rxns WILL_BE stoic_reaction_set`
- `xi[rxns] IS_A molar_rate`
- `Qdot IS_A power`
- `DeltaP IS_A pressure`

Required aliasing/consistency:

- inlet and outlet must share the same `pkg`
- inlet and outlet component sets should alias the package stream basis
- stoichiometric coefficients must be defined on that same basis

First balance set:

- component balances:
  `outlet.f[k] = inlet.f[k] + SUM(nu[k,r] * xi[r])`
- pressure relation:
  `outlet.P = inlet.P - DeltaP`
- energy balance:
  `SUM_in(Hdot) + Qdot = SUM_out(Hdot)`

For the first version there should be no attempt to infer extents from
conversion specifications automatically. Extents should be the primary
variables; user-friendly conversion specifications can be layered on
later.

The first version should also be steady-state only. Dynamic holdup terms
are a later extension.

### 14.7.3 Required C blackbox/property work for `reactor_stoic`

`reactor_stoic` does not need equilibrium closure, but it does require a
reliable package-backed enthalpy/property path.

Minimum required C-side capabilities:

- runtime package construction and caching from A4 package constants
- package-backed stream enthalpy:
  `fprops_rxn_mix_h(pkg, state, &H)`
- ASCEND blackbox wrapper that accepts:
  - package handle/key
  - `T`
  - `P`
  - species molar flow vector on package basis
  and returns:
  - total molar enthalpy flow or molar enthalpy, depending on chosen A4
    convention

The wrapper should not embed stoichiometric logic. Reaction extents and
species balances belong in A4. The C side should only provide the
property closure.

This is important architecturally because it keeps `reactor_stoic`
usable with:

- fixed extents
- conversion specs
- externally computed extents
- later kinetic source models

without changing the property API.

### 14.7.4 First delivery limitations for `reactor_stoic`

The first delivery should explicitly exclude:

- general flashed multiphase outlet states
- nonideal liquid mixture enthalpy such as UNIFAC-based liquid mixtures
- generic solution-phase enthalpy for wustite/spinel-rich mixed streams
- automatic reaction parsing from chemistry strings

The first successful target should instead be:

- gas species
- pure condensed species
- one canonical package basis shared by stream and reaction data

That is enough to support a serious first Fe-O-H reduction model and a
water-gas-shift validation case.

### 14.7.5 Recommended order of implementation tasks

The first concrete work packages should be:

1. add A4 package/state/stream skeletons with no reactor logic yet
2. add A4 `stoic_reaction_set` and `reactor_stoic` skeletons with
   species balances only
3. bind package-backed enthalpy as an ASCEND blackbox and wire the
   reactor energy balance
4. add one minimal gas-phase regression model such as WGS
5. add one Fe-O-H step-reaction regression model using pure solids and
   gas species
6. only after those pass, begin `reactor_equil`

This sequence forces early settlement of:

- package identity sharing across connected units
- species-basis discipline
- heat-duty sign convention

### 14.7.6 ASCEND compiler and tooling issues observed

During the first `reactor_stoic` prototype, a few ASCEND-side issues
were encountered that should be treated as tooling/compiler bugs or at
least as areas needing clearer diagnostics.

Observed issues:

- `ARE_THE_SAME` inside `FOR ... CREATE` produced an internal
  `Pass2ExecuteForStatements` assertion failure when the aliased arrays
  were unconformable, instead of reporting the actual user-level
  conformability error. The same mismatch, expressed outside the `FOR`
  loop, produced a normal diagnostic. This suggests a compiler error
  handling bug in pass 2 around `ARE_THE_SAME` expansion within
  generated statements.
- method-time assignment to `symbol_constant` package fields such as
  `source := ''` in `default_self` was rejected as assignment to a
  constant instance. Declarative assignment using `:==` on the instance
  declaration side works normally. It is not yet clear whether this is
  intended language behaviour, an inconsistency in constant handling, or
  simply a gap in documentation, but it is important because package
  configuration naturally wants to live in model initialization methods.
- declarative relations inside `FOR ... CREATE` that referenced
  `real_constant` stoichiometric arrays such as `nu[k]` or
  `nu[k][r]` caused an internal `Pass2ExecuteForStatements` assertion
  failure. The underlying relation error is that these constants are
  still unassigned at pass 2, so they cannot appear in compiled
  relations yet; the real bug was the crash path rather than the
  rejection itself. A minimal reproducer now lives in
  `models/test/compiler/forsum_bug.a4c` (`forsum_bug_const_matrix`),
  and the reported user-level error is:

  `relation: Unassigned constants or wild dimensioned real constant in relation`

  In other words, method-time assignment to `real_constant`
  stoichiometric coefficients is too late for relation compilation.
  Using fixed `solver_var` coefficients (for example `factor`) or
  declarative `:==` assignment avoids the issue. Declarative `TABLE`
  assignment also works, because `TABLE` statements execute early enough
  during instantiation to populate the constant matrix before pass-2
  relation compilation. A minimal proof-of-concept model is
  `tmp/table_const_matrix.a4c`.

Current workarounds:

- avoid using `ARE_THE_SAME` inside `FOR ... CREATE` when whole-array
  aliasing or simpler direct declarations can express the same intent
- prefer declarative `:==` initialization for package constants such as
  `species_name[...]` and `source`
- use symbol-indexed blackbox vector passing directly now that
  `y[components] : INPUT` support has been added in the compiler
- represent stoichiometric coefficients as fixed `solver_var` arrays
  rather than `real_constant` arrays when they must appear in generated
  declarative relations
- if truly constant stoichiometric coefficients are desired in future,
  they must be assigned declaratively early enough to be available at
  pass 2, or ASCEND’s relation compiler will need an explicit delayed
  constant-resolution mechanism

These should be revisited separately from the thermo/reactor design
work, because they affect general ASCEND model ergonomics beyond this
RFC.

### 14.7.7 First regression and demo ladder

The first tests should be split across three levels.

Kernel/property tests:

- package-backed enthalpy for small gas mixtures
- package-backed enthalpy for pure condensed species used in Fe-O-H
- package build and evaluation error handling

A4 unit-model tests:

- one-reaction WGS stoichiometric reactor, isothermal
- one-reaction WGS stoichiometric reactor, adiabatic
- Fe2O3 + H2 -> Fe3O4 + H2O step reactor
- Fe3O4 + H2 -> FeO + H2O step reactor
- FeO + H2 -> Fe + H2O step reactor

Flowsheet/demo tests:

- two stoichiometric reactors in series for staged reduction
- optional heater plus stoichiometric reactor to test coupled energy
  balance and temperature change

This ladder is preferable to jumping directly to a full direct-reduction
flowsheet, because each stage isolates one new layer of the design.

## 14.8 Constraints to avoid painting the design into a corner

The first implementation should be intentionally narrow, but it must
respect a few rules so that later generalization remains straightforward.

### 14.8.1 Keep one canonical species basis per package

For any rigorous `reactive_package`, there should be one canonical
species/endmember basis known to the runtime package and used
consistently by:

- `reactor_stoic`
- `reactor_equil`
- stream state property evaluation
- later flash or separator models

Different black-boxes may expose different convenience wrappers, but
they should all reduce to this same internal basis.

### 14.8.2 Do not make `fprops_eqm_tpy` responsible for general flash logic

`fprops_eqm_tpy` should remain a convenience wrapper:

- infer element totals from inlet species amounts
- call the `TPb` equilibrium primitive on the supplied species basis

It should not be burdened with automatic phase splitting, outlet
routing, or general multiphase stream interpretation.

If later users need true flash-style semantics, that should be a
separate API and a separate unit-model closure.

### 14.8.3 Keep simple enthalpy summation separate from flash/state objects

`fprops_mix_h_tpn` should remain the simple primitive:

`H = SUM[n_i * h_i(T,P)]`

This is enough for:

- gas mixtures
- pure condensed species
- explicit endmember lists used in Fe-O-H
- stoichiometric and equilibrium reactors that already know the species
  amounts

Later multiphase or nonideal liquid enthalpy should be built as
higher-level state/property APIs rather than by overloading this
primitive with flash responsibilities.

### 14.8.4 Treat nonideal liquids as a later package capability, not a hidden assumption

UNIFAC and similar models are important, but they are not on the
critical path for hydrogen reduction of iron oxides. Therefore the
first reactor/property stack should not assume that all future packages
behave like ideal or explicit-endmember systems.

The package abstraction should leave room for later support of:

- activity-coefficient liquid phases
- cubic-EOS gas/liquid fugacity models
- richer phase-state objects with explicit phase fractions and
  compositions

without requiring that those capabilities exist in the first delivered
milestone.

## 15. Fe-O-H as the First Demonstrator

The current Fe-O-H capability in `fprops` is a strong candidate for the
first rigorous reactive package because:

- the equilibrium kernel already supports its species/endmember set
- validation work already exists
- it exercises gas species, pure condensed species, and solution phases
- it forces the design to cope with real internal basis complexity

However, Fe-O-H is not necessarily the best first performance target for
kernel bring-up because it is already relatively slow. Early testing of
`fprops_eqm_tpb` may benefit from faster systems such as:

- water-gas shift
- methane reforming subsets
- small ideal-gas reacting systems

Those can give faster regression cycles while Fe-O-H remains a primary
application driver.

This suggests a validation ladder rather than a single heroic
demonstrator:

1. water-gas shift equilibrium against reference `K(T)` data
2. Fe-O-H equilibrium at fixed `T`, `P`
3. `reactor_stoic` with Fe-O-H step reactions and energy balance
4. `reactor_equil` on the same package and stream basis
5. only later, reactive flash or downstream phase-splitting models

An initial Fe-O-H demonstrator package should probably expose the full
internal basis on streams, with additional derived outputs such as:

- metallic iron total
- wustite total
- wustite composition
- spinel total
- gas composition summaries

## 16. Open Design Questions

The following questions remain open and should be answered during the
next design stage.

### 16.1 Exact A4 syntax of `reactive_package`

This RFC deliberately does not commit to final A4 syntax yet.

### 16.2 Whether elements ever need to be exposed in A4

The current direction is to keep them hidden in the runtime package, but
it remains open whether some later debugging or advanced-modeling use
cases would benefit from explicit A4 exposure.

### 16.3 Whether `reactor_stoic` should use a dedicated reaction package

This may be cleaner than mixing stoichiometric reaction data into the
same object as thermodynamic package data.

### 16.4 Whether a generic `reactive_state` property black-box is enough

If later property work requires richer phase detail, additional
black-boxes may be needed.

### 16.5 Whether some future packages can safely use coarser stream bases

This will depend on whether auxiliary descriptors are designed well
enough to transport the missing internal state.

### 16.6 Species alias and formula-based name resolution

The current `reactive_package` examples require explicit mappings such
as `CO -> carbonmonoxide` and `CO2 -> carbondioxide`, which is not a
good long-term user experience.

A likely direction is:

- keep one canonical internal FPROPS species name per basis species
- make `species_name[...]` an optional explicit override
- otherwise resolve the user-facing component token via:
  - exact canonical-name match
  - normalized-name match (`carbon_dioxide`, `carbon-dioxide`, etc)
  - a built-in alias table (`CO`, `CO2`, `H2`, `H2O`, ...)
  - formula lookup, but only when the match is unique

This would allow simple reactor models to use familiar chemical symbols
by default, while still requiring explicit `species_name[...]`
assignment for ambiguous cases such as hydrogen variants, phase-specific
species, solution members, wustite/spinel members, and any future
multiphase basis species.

This should be implemented as a package-build-time resolver in FPROPS,
not as a string rewrite scattered through ASCEND MODEL code.

### 16.7 Generated C source-data path for UNIFAC and future mixture models

The current UNIFAC flash implementation proves the thermodynamics, but
its data path is not yet architecturally right: FPROPS is reading
immutable mixture database data back out of the ASCEND instance tree via
`components.a4l`.

That is acceptable as a bring-up and verification path, but it should
not be the long-term design.

The preferred end state is analogous to the existing
`filedata.h` -> `rundata.h` -> prepared-evaluator pattern already used
for pure-fluid data:

1. source data stored in generated `const` C structs
2. runtime evaluators prepared from those source structs
3. ASCEND passing only names/selectors and state variables
4. `components.a4l` retained only as a verification/reference route
   during migration

#### 16.7.1 Separation of concerns

The desired split is:

- `filedata`-like layer:
  immutable generated source records
- `rundata`-like layer:
  prepared runtime objects and caches
- ASCEND:
  user-facing names, model structure, and solve-time state only

This means that UNIFAC data should not be "owned" by ASCEND models.
Instead, ASCEND should identify the requested mixture package and
components, while FPROPS owns:

- database lookup
- name/alias resolution
- source-data selection
- runtime preparation
- prepared-cache lifetime

#### 16.7.2 Proposed directory and file layout

The existing `fluids/` directory is appropriate for pure-component
source data, but activity-coefficient and group-contribution models are
better treated as a parallel class of source data rather than forcing
them into the pure-fluid directory.

A sensible first layout is:

```text
fprops/
  filedata.h
  rundata.h
  mixtures/
    unifac_data.h
    unifac_data.c
    unifac_rundata.h
    unifac.c
    _unifac_groups.c
    _unifac_components.c
    _unifac_lookup.c
    convunifac.py
```

The intent is:

- `_unifac_groups.c`, `_unifac_components.c`, `_unifac_lookup.c`
  are generated source-data files
- `unifac_data.h`
  declares source-data structs and lookup functions
- `unifac_rundata.h`
  declares prepared/runtime UNIFAC objects
- `unifac.c`
  converts source data into prepared runtime form and evaluates
  activities/excess properties
- `convunifac.py`
  is the first generator, separate from `convcomp.py`

It is possible that `convcomp.py` could eventually call a shared parser
library and emit both pure-fluid and UNIFAC artifacts, but the initial
code generator should be kept separate because the emitted data models
are materially different.

#### 16.7.3 Proposed generated source-data structs

At source-data level, FPROPS needs at least:

- subgroup definitions
- main-group interaction parameters
- component-to-subgroup mapping
- canonical component names
- optional aliases
- optional pure-component auxiliary data used by flash calculations

One workable C model is:

```c
typedef struct{
	const char *name;      /* e.g. "OH", "H2O", "CH3" */
	int subgroup_id;       /* stable generated ID */
	int main_group_id;     /* UNIFAC main group */
	double R;
	double Q;
} UNIFACSubgroupData;

typedef struct{
	int subgroup_index;    /* index into subgroup table */
	double nu;             /* stoichiometric count in the component */
} UNIFACComponentSubgroup;

typedef struct{
	const char *name;      /* canonical FPROPS component name */
	const char **aliases;  /* optional, NULL-terminated or counted */
	int naliases;

	int nsubgroups;
	const UNIFACComponentSubgroup *subgroups;

	double r;              /* precomputed sum(nu_i * R_i) */
	double q;              /* precomputed sum(nu_i * Q_i) */

	/* optional pure-component data for current TPz flash kernel */
	double Tc, Pc, omega;
	double T0, P0, H0, G0;
	double cpvapa, cpvapb, cpvapc, cpvapd;
	double Vliq, Tliq;
	int vp_correlation;
	double vpa, vpb, vpc, vpd;
} UNIFACComponentData;

typedef struct{
	int ngroups;                 /* dense storage, current original UNIFAC uses 47 */
	const double *aij;           /* row-major ngroups x ngroups */
} UNIFACInteractionData;
```

A top-level package descriptor can then bind these together:

```c
typedef struct{
	const char *name; /* e.g. "UNIFAC-orig-2003" */
	const UNIFACSubgroupData *subgroups;
	int nsubgroups;
	const UNIFACComponentData *components;
	int ncomponents;
	const UNIFACInteractionData *interactions;
} UNIFACSourceData;
```

This is the mixture analogue of the pure-fluid source-data layer.

#### 16.7.4 Proposed prepared/runtime structs

The runtime layer should not simply re-expose the generated arrays.
Preparation should compact, validate, and cache what the evaluator needs
most often.

One likely runtime object is:

```c
typedef struct{
	const UNIFACSourceData *src;

	int nc;
	const UNIFACComponentData **components; /* chosen components */

	int nactive_subgroups;
	const UNIFACSubgroupData **active_subgroups;

	double *aij_dense;   /* compact dense interaction matrix over active main groups */
	double *r;           /* per-component r */
	double *q;           /* per-component q */

	/* optional cached work buffers or scratch allocators later */
} UNIFACRunData;
```

This is the correct place to:

- resolve component names once
- validate subgroup completeness once
- reduce the full database to the active component subset
- compact the active interaction matrix
- precompute any repeated sums or mappings

That is directly analogous to preparing a `PureFluid *` from `EosData`.

#### 16.7.5 Proposed public/runtime API

The mixture-data path should offer a package-style front door similar to
the new reactive package work:

```c
const UNIFACComponentData *fprops_unifac_component(
	const char *name
);

const UNIFACSourceData *fprops_unifac_source(
	const char *model_name
);

UNIFACRunData *fprops_unifac_prepare(
	const UNIFACSourceData *src,
	const char **components,
	int nc
);

void fprops_unifac_destroy(UNIFACRunData *run);
```

and then evaluation routines such as:

```c
int fprops_unifac_gamma_run(
	const UNIFACRunData *run,
	double T,
	const double *x,
	double *gamma
);
```

The current flash/equilibrium wrappers should ultimately depend on
prepared runtime objects of this kind, not on ASCEND-instance data
extraction.

#### 16.7.6 Generator responsibilities

The generator should read the canonical source definitions now present in
`components.a4l` and emit:

- subgroup table with stable indices
- main-group interaction matrix
- per-component subgroup composition arrays
- optional alias table
- optional lookup tables by canonical name
- optional pure-component auxiliary constants used by current flash code

The generator does not need to emit prepared/runtime objects. It should
emit only immutable source data.

That keeps the model clean:

- generation step:
  translate ASCEND source database to C source-data literals
- runtime step:
  prepare compact evaluators from those literals

#### 16.7.7 Relation to `_rpp.c` and `convcomp.py`

The existing `_rpp.c` pattern is the right precedent in spirit:

- ASCEND source definitions
- code generation into `const` C records
- normal FPROPS preparation from those source records

But UNIFAC is not just "one more pure fluid". It is a separate class of
thermodynamic database. Therefore the first implementation should use a
separate generator, likely `convunifac.py`, rather than overloading
`convcomp.py`.

Later, if desirable, the parsing logic for `components.a4l` can be
shared between the generators.

#### 16.7.8 Verification and migration strategy

The current `components.a4l` extraction path remains valuable during the
transition, because it gives an independent route to the same
thermodynamic information.

The recommended migration path is:

1. generate C-native UNIFAC source data from `components.a4l`
2. add C-native prepare/evaluate path alongside the current
   ASCEND-instance route
3. run both paths against the same tests:
   - direct DDBST `gamma(T,x)` checks
   - ASCEND phase-model equality checks
   - ethanol-water TPz flash benchmark
4. once parity is demonstrated, make the C-native route the default
5. later, remove the `components.a4l` dependency from the FPROPS flash
   kernel

This should be treated as a controlled migration, not a flag day.

#### 16.7.9 Why this helps flash and equilibrium converge

Once mixture data are native to FPROPS in the same way as pure-fluid
data, `flash` and `eqm` can converge on the same general package
concept:

- generated source-data registry
- prepared runtime package
- user-facing name resolution
- state/equilibrium solver on top

At that point, the main difference between `flash` and `eqm` becomes the
state problem being solved, not the origin of the thermodynamic data.

That is the correct architectural direction for broader future work such
as:

- cubic-EOS fugacity phases
- UNIFAC/NRTL liquid phases
- reactive flashes
- solids plus solution phases in one package

## 17. Recommendation

The recommended architectural direction is:

1. Add a new `reactive_package` abstraction with a light A4 identity and
   a heavy C-side runtime cache.
2. Add a parallel `reactive_state` / `reactive_stream` family rather
   than retrofit the old thermo stack immediately.
3. Use elemental conservation as the balance basis for equilibrium
   reactors.
4. Expose a thermodynamic black-box of the form
   `Phi(pkg, T, P, elemental basis)`.
5. Build `reactor_equil` and `reactor_stoic` first.
6. Treat report groups and nonlinear descriptors as reporting aids
   before using them as the transport basis.
7. Prioritize first-derivative support for equilibrium black-boxes as an
   explicit adoption requirement.

That path is technically consistent with the existing `fprops/eqm`
implementation, does not overload the old ASCEND thermo models, and is
extensible toward a broader thermodynamic database and more sophisticated
reactive unit operations later.

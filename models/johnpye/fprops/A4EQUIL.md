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
  [models/thermodynamics.a4l](/home/john/ascend/models/thermodynamics.a4l)
- legacy process unit models such as
  [models/stream_holdup.a4l](/home/john/ascend/models/stream_holdup.a4l)
  and [models/reactor.a4l](/home/john/ascend/models/reactor.a4l)
- a newer Gibbs-minimization equilibrium kernel in
  [models/johnpye/fprops/eqm.h](/home/john/ascend/models/johnpye/fprops/eqm.h)
  and [models/johnpye/fprops/eqm.c](/home/john/ascend/models/johnpye/fprops/eqm.c)

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
- possibly the element set when element arrays appear explicitly in A4

Heavy thermodynamic detail should remain in a C-side runtime package
cache.

### 2.4 Keep the first deliverables narrow

The first rigorous units should be:

- `reactor_equil`: a single mixed-outlet equilibrium reactor
- `reactor_stoic`: a stoichiometric reactor using the same reactive
  thermodynamic package system

Phase-splitting units such as a reactive flash should come later.

## 2A. Existing Reactor Library Context

Before proposing a new reactive stack, it is useful to record what the
existing reactor library already provides.

The current reactor library in
[models/reactor.a4l](/home/john/ascend/models/reactor.a4l) is a
kinetics-based CSTR library, not a chemical-equilibrium library.

It provides two reactor models:

- `single_phase_cstr` in
  [models/reactor.a4l](/home/john/ascend/models/reactor.a4l#L47)
- `multiple_phase_cstr` in
  [models/reactor.a4l](/home/john/ascend/models/reactor.a4l#L245)

These models rely on the rate-based source-term interface from
[models/kinetics.a4l](/home/john/ascend/models/kinetics.a4l), especially
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
[models/reactor.a4s](/home/john/ascend/models/reactor.a4s#L46), where
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

### 3.4 Report groups

These are named linear aggregations of internal species used for
reporting or GUI display:

`n_report[g] = SUM[R[g,i] * n_internal[i]]`

Examples:

- `metal_iron = Fe_bcc + Fe_fcc`
- `wustite_total = Wus_FeO + Wus_FeO1p5`
- `spinel_total = Sp_Fe2_tet + Sp_Fe3_tet + Sp_Fe2_oct + Sp_Fe3_oct + Sp_Va_oct`

They are not initially proposed as the transport basis for streams.

### 3.5 Nonlinear descriptors

Some useful reported quantities are not linear sums and therefore are
not report groups. Examples:

- `x_wus = Wus_FeO1p5 / (Wus_FeO + Wus_FeO1p5)`
- spinel site fractions
- phase fractions

These should be treated as derived outputs, not part of the primary
stream basis.

## 4. Proposed `reactive_package`

## 4.1 Role

The `reactive_package` defines the chemical and thermodynamic universe
for a set of connected reactive units.

It answers questions such as:

- what stream components exist?
- what conserved elements exist?
- what internal equilibrium species exist?
- which thermo models and source maps are used?
- how are stream components mapped to elements?
- how are internal species mapped to report groups?

The package is the object that should be shared with
`WILL_BE_THE_SAME`-style constraints across a reactive flowpath.

## 4.2 Hybrid A4/C design

The package should exist in two forms.

### ASCEND-side package object

This is intentionally light. It should expose only what ASCEND needs to
instantiate arrays and validate connections:

- `stream_components`
- optionally `elements`
- `package_name`
- `package_version`
- `package_key`

The `package_key` should be a stable identity or hash, not a raw pointer.

### C-side runtime package

This is the heavy representation built and cached behind the package key.
It contains:

- internal equilibrium species
- elemental incidence matrices
- thermo source maps
- phase model definitions
- solution-phase metadata
- report groups
- nonlinear descriptor definitions
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
- `V` optionally later
- `phase_summary` outputs optionally later

In the first implementation, `reactive_state` should not try to encode
all internal equilibrium detail as A4 arrays. Instead, its thermodynamic
properties are supplied by black-box relations using the package key.

## 5.2 `reactive_stream`

This parallels `detailed_stream` in
[models/stream_holdup.a4l](/home/john/ascend/models/stream_holdup.a4l).

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

## 6.2 Internal equilibrium solve

The runtime package provides an internal species elemental matrix:

`A_internal[e,i]`

The FPROPS kernel then solves over `n_internal[i]` or `f_internal[i]`.

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
- use report groups and descriptors only for display and reporting

This preserves information exactly and keeps downstream units consistent.

### Later extension

Once there is a robust notion of package-specific pseudo-components plus
auxiliary descriptors, a coarser stream basis may be introduced.

## 7. Black-Box Function Strategy

The existing `fprops` integration in
[models/johnpye/fprops/asc_fprops.c](/home/john/ascend/models/johnpye/fprops/asc_fprops.c)
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

The names below are placeholders for the RFC. Exact naming can be
changed later.

### `a4equil_tp_b`

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
- optional phase summary outputs
- optional internal species outputs when stream basis is coarser than
  internal basis

Data:

- `reactive_package`

Internally this black-box:

1. converts package key to runtime package
2. builds/uses `A_internal`
3. calls `eqm_solve_elements(...)` or a package-aware extension
4. evaluates equilibrium enthalpy from the resulting equilibrium state
5. maps outputs to stream basis

### `a4rxnprops_tp_f`

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

## 7.3 Derivative support

If reactive units are to solve robustly inside ASCEND, these black-boxes
need derivative callbacks, not only residual evaluation.

For `a4rxnprops_tp_f`, standard black-box derivative support should be
added directly if the property model can supply it.

For `a4equil_tp_b`, derivatives are more subtle because the equilibrium
state is itself the result of an optimization/KKT solve. The correct
approach is implicit differentiation of the equilibrium conditions, not
naive finite differencing in the long term.

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

## 8.3 Governing balances

For a single-inlet, single-outlet, steady reactor with no shaft work:

### Element balances

For each element `e`:

`b_dot_in[e] = b_dot_out[e]`

with:

`b_dot_in[e] = SUM[A_stream[e,k] * inlet.f[k]]`

and similarly for the outlet.

### Pressure relation

For example:

`outlet.P = inlet.P - dP`

### Equilibrium closure

At the outlet state:

`(outlet.f[components], H_out, aux) = a4equil_tp_b(outlet.T, outlet.P, b_dot_in, pkg)`

In practice the black-box may be written as a set of relations equating
ASCEND variables to the returned equilibrium outputs.

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

`outlet.state.H = a4rxnprops_tp_f(outlet.T, outlet.P, outlet.f[components], pkg)`

Energy balance is then written exactly as for the equilibrium reactor.

## 9.5 Stoichiometric reaction basis

Unlike equilibrium, stoichiometric reactions do depend on a chosen
reaction basis. That is acceptable here because it is a user-specified
reactor model, not a universal thermodynamic closure.

## 10. Governing Thermodynamic Basis Inside FPROPS

The equilibrium kernel in
[models/johnpye/fprops/eqm.c](/home/john/ascend/models/johnpye/fprops/eqm.c)
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

For `a4equil_tp_b`, the important sensitivities are:

- `d f_eq / dT`
- `d f_eq / dP`
- `d f_eq / db`
- `d H_eq / dT`
- `d H_eq / dP`
- `d H_eq / db`

These should be obtained by linearizing the equilibrium KKT system and
solving the resulting sensitivity system.

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

### 14.1 New package and stream family

- add `reactive_package`
- add `reactive_state`
- add `reactive_stream`

### 14.2 Black-boxes

- implement `a4equil_tp_b`
- implement `a4rxnprops_tp_f`

### 14.3 First reactor models

- implement `reactor_equil`
- implement `reactor_stoic`

### 14.4 Initial package discipline

- for rigorous packages with solution phases, set
  `stream_components = internal equilibrium species`
- report friendly totals/descriptors separately

### 14.5 Leave for later

- reactive flash / phase-splitting unit
- pseudo-component transport with auxiliary descriptors
- dynamic reactive holdups
- PFR discretizations
- metastable constraints

## 15. Fe-O-H as the First Demonstrator

The current Fe-O-H capability in `fprops` is a strong candidate for the
first rigorous reactive package because:

- the equilibrium kernel already supports its species/endmember set
- validation work already exists
- it exercises gas species, pure condensed species, and solution phases
- it forces the design to cope with real internal basis complexity

An initial Fe-O-H demonstrator package should probably expose the full
internal basis on streams, with additional report outputs such as:

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

### 16.2 How much of the package element set should be exposed in A4?

Exposing elements makes some unit equations clearer, but keeping them
hidden makes the instance tree lighter.

### 16.3 Whether `reactor_stoic` should use a dedicated reaction package

This may be cleaner than mixing stoichiometric reaction data into the
same object as thermodynamic package data.

### 16.4 Whether a generic `reactive_state` property black-box is enough

If later property work requires richer phase detail, additional
black-boxes may be needed.

### 16.5 Whether some future packages can safely use coarser stream bases

This will depend on whether auxiliary descriptors are designed well
enough to transport the missing internal state.

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

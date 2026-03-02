# FPROPS Fe-O Implementation Note

This note tracks the implementation plan for Fe-O equilibrium in `fprops`,
with emphasis on Tier 2 and on the nonstoichiometric wustite phase.

The immediate goal is not full CALPHAD generality.
The goal is a minimal, technically sound extension of the current Gibbs
minimization engine so that:

- stoichiometric Fe, Fe3O4, and Fe2O3 can coexist consistently,
- wustite can appear as a condensed solution phase with variable composition,
- the resulting framework can be reused later for Fe-O-H and Fe-O-C-H.

## 1. Tier 2 Scope

Tier 2 is the Fe-O-only equilibrium layer:

- `Fe(s)`
- `Fe3O4(s)`
- `Fe2O3(s)`
- wustite as a nonstoichiometric solid solution

We explicitly do **not** aim, in the first implementation, to add:

- a full compound-energy-formalism framework,
- defect chemistry with explicit Fe2+/Fe3+/vacancies/interstitials,
- generic multicomponent condensed-solution infrastructure.

Instead, we aim for one robust binary condensed-solution phase that is
sufficient to represent wustite.

### 1.1. Pragmatic Tier 2 Phase Set

For the first complete Tier 2 implementation in `fprops`, we adopt the
following reduced phase set:

- `Fe_bcc`
- `Fe_fcc`
- `Fe3O4`
- `Fe2O3`
- wustite as the binary solution phase
  $$
  (1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{3/2}
  $$

This is intentionally **not** the full Hidayat assessment.

The pragmatic simplifications are:

- iron is treated as stoichiometric pure condensed phases
  (`Fe_bcc`, `Fe_fcc`), not as Fe-O substitutional solutions,
- magnetite is treated as a stoichiometric compound using its fitted
  Gibbs-energy line only,
- hematite is treated as a stoichiometric compound using its fitted
  Gibbs-energy line only,
- the magnetic terms reported in Hidayat are deferred in the first pass,
- the full magnetite CEF inherited from Ref. [8] is deferred,
- oxygen dissolved in metallic iron is deferred.

This reduced set is sufficient to:

- test coexistence of multiple condensed Fe-O phases,
- recover a first useful Fe-O equilibrium backbone,
- support the next Tier 3 step toward Fe-O-H,
- avoid overextending the thermodynamic framework before it is validated.

The tradeoff is that the first Tier 2 implementation is expected to be
qualitatively useful but not yet a faithful reproduction of the full Hidayat
Fe-O assessment.

### 1.2. Validation Status For The Pragmatic Tier 2 Subset

At the current stage, the Tier 2 Fe-O work should be understood in three
levels.

Implemented and directly validated:

- the accepted Hidayat binary wustite model
  $$
  (1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{3/2}
  $$
- full-space Gibbs minimization with a condensed binary solution phase,
- direct Gibbs-energy lookup for the pragmatic Hidayat subset
  (`Fe_bcc`, `Fe_fcc`, `Fe3O4`, `Fe2O3`),
- unary Hillert-Jarl magnetic terms for `Fe_bcc`, `Fe_fcc`, and `Fe2O3`,
- element accounting for mixed stoichiometric phases plus solution-phase
  endmembers,
- low-oxygen Fe-O equilibrium smoke cases with metal plus wustite present,
- direct `\mu^\circ(T)` checks against the Hillert-Jarl formula for
  `Fe_bcc`, `Fe_fcc`, and `Fe2O3`,
- rejection of reduced/nullspace solution paths for systems containing
  solution phases.

Implemented but only pragmatically supported:

- `Fe_bcc` and `Fe_fcc` as pure condensed species from the Hidayat Gibbs lines,
- `Fe3O4` as a stoichiometric compound using only the published Gibbs line,
- `Fe2O3` as a stoichiometric compound using only the published Gibbs line.

Not yet validated against the full Hidayat paper:

- quantitative `Fe | wustite` oxygen-potential boundaries,
- quantitative `wustite | Fe3O4` boundaries,
- quantitative `Fe3O4 | Fe2O3` boundaries,
- the full Fe-O phase diagram shapes shown in Hidayat figures.

Explicitly deferred:

- the full magnetite CEF inherited from Hidayat Ref. [8],
- dissolved oxygen in metallic iron,
- a claim of quantitative reproduction of the published Hidayat Fe-O
  boundaries.

So the current validation claim should be phrased as:

- the wustite solution model and the solution-phase minimization framework are
  implemented and tested,
- the surrounding Fe-O phase set is present in a pragmatic reduced form,
- stronger comparison to Hidayat figures should wait until the deferred
  magnetic / CEF pieces are added.

### 1.3. Magnetic-Model Provenance

The relevant literature chain for the magnetic contribution is:

- Hidayat et al. (2015) for the Fe-O reassessment and for the statement that
  the properties of spinel and hematite were optimized earlier in Ref. [8].
- Degterov et al. (2001), Ref. [8] in Hidayat, for the detailed spinel/hematite
  modeling context.
- Hillert and Jarl (1978) for the standard CALPHAD magnetic Gibbs-energy model.
- Hertzman and Sundman / Weiss-Tauer convention for the antiferromagnetic
  scaling used in the standard CALPHAD treatment.

What Hidayat says directly is limited but important:

- the major solid solutions are wustite and spinel,
- fcc iron, bcc iron, and hematite have very narrow nonstoichiometry ranges,
- the spinel and hematite properties were taken largely from the earlier
  Degterov assessment and only adjusted slightly for consistency with the new
  wustite and liquid.
- hematite is antiferromagnetic below a Neel temperature of 956 K,
- Table 1 reports magnetic parameters for `Fe_bcc`, `Fe_fcc`, and `Fe2O3`
  using the standard CALPHAD triplet:
  magnetic ordering temperature, magnetic moment `\beta`, and
  structure-dependent parameter `P`.

What Degterov says directly is the crucial magnetic-model pointer:

- the spinel solid solution is modeled with the Compound Energy Formalism,
- the magnetic contribution is described by combining that CEF model with the
  Hillert-Jarl magnetic equation,
- pseudocomponents and magnetic interaction parameters must be assigned so that
  the resulting magnetic properties are physically reasonable.

The OCR text from Degterov states this directly:

- "The phenomenological approach proposed by Hillert and Jarl was used
  to describe the magnetic contribution to the thermodynamic functions."
- "The magnetic contribution to the Gibbs energy is given by Eq. [26]."

The OCR text from Hidayat gives the implementation-level inheritance:

- "The properties of spinel and hematite were optimized earlier [8] and only
  small adjustments were required to make them completely consistent with the
  new wustite and liquid."
- "The thermodynamic properties of magnetite, Fe3-xO4, and hematite, Fe2O3,
  were slightly changed as compared to the earlier optimization [8] ..."

So the academically grounded reading is:

- Hidayat does **not** introduce a new magnetic model for Fe-O,
- Hidayat reuses the established CALPHAD magnetic machinery and the earlier
  Degterov spinel/hematite assessment,
- therefore the correct literature basis for implementation is the standard
  Hillert-Jarl magnetic term, not an ad hoc software-specific convention.

For `fprops`, that leads to a staged recommendation:

- first add the standard unary magnetic correction to the pragmatic species
  `Fe_bcc`, `Fe_fcc`, and `Fe2O3`,
- continue to defer the full spinel CEF + magnetic parameterization,
- only revisit full magnetite fidelity if later validation against Hidayat
  figures shows that the stoichiometric `Fe3O4` approximation is insufficient.

In practical terms, the machinery required for the next step is still light:

- one reusable implementation of the standard Hillert-Jarl unary magnetic term,
- parameters `(T_C \text{ or } T_N, \beta, P)` for the affected phases,
- additive use of that term in the existing direct-Gibbs pragmatic species.

The heavy machinery remains deferred:

- full CEF + magnetic treatment for spinel,
- composition-dependent magnetic properties for condensed solution phases,
- any attempt to reproduce the full inherited magnetite parameterization from
  Degterov in the first pass.

### 1.5. First Quantitative Validation Set

The first quantitative Fe-O validation target for the reduced Tier 2 model is
the solid-state eutectoid reported in Hidayat Table 2:

$$
\mathrm{Wustite} \rightarrow \mathrm{Fe(bcc)} + \mathrm{Magnetite}
$$

at:

$$
T = 561~^\circ\mathrm{C} = 834.15~\mathrm{K}
$$

with the wustite composition reported as:

$$
51.4~\text{at\% O}
$$

For the endmember parameterization

$$
(1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{3/2}
$$

that composition corresponds to:

$$
x = 0.115226
$$

The current diagnostic script is:

- `models/johnpye/fprops/test/feo_hidayat_validation.py`

For the reduced Tier 2 model currently implemented in `fprops`, the coexistence
conditions at that Hidayat target point are:

$$
g_{\mathrm{Fe(bcc)}} = 3\mu_A - 2\mu_B
$$

$$
g_{\mathrm{Fe_3O_4}} = \mu_A + 2\mu_B
$$

The present diagnostic result is:

- at the Hidayat target point `(T = 834.15 K, x = 0.115226)`,
  the `Fe(bcc) | wustite` residual is about `-15.146 kJ/mol`,
- at the same point, the `wustite | magnetite` residual is about
  `+32.803 kJ/mol`,
- at `834.15 K`, the reduced model can satisfy the `Fe(bcc) | wustite`
  condition only near `x = 0.362772` (`54.158 at% O`),
- at `834.15 K`, the best `wustite | magnetite` fit is still poor:
  near `x = 0.666740` (`57.144 at% O`) with about `7.967 kJ/mol`
  residual.

So the first quantitative conclusion is clear:

- the reduced model does **not** yet reproduce the Hidayat eutectoid
  invariant in Table 2,
- the dominant remaining mismatch is the magnetite side of the boundary,
- this is consistent with the fact that the full Degterov/Hidayat spinel
  CEF + magnetic treatment is still deferred.

This is a useful checkpoint rather than a failure of the architecture:

- the wustite solution machinery is working,
- the unary magnetic correction is working,
- and the first quantitative comparison now identifies where the reduced model
  stops being faithful to the published Fe-O assessment.

After replacing the stoichiometric magnetite surrogate with the Fe-only slice
of the Degterov spinel model, the magnetite-side comparison improves
materially:

- at the same Hidayat target point, the `wustite | spinel` grand-potential
  residual drops to about `+12.738 kJ/mol`,
- the best-fit `wustite | spinel` boundary at `834.15 K` moves to about
  `x = 0.438040` (`54.935 at% O`),
- the best-fit `Fe(bcc) | wustite` boundary remains near
  `x = 0.362558` (`54.155 at% O`).

So the Degterov spinel upgrade clearly improves the oxide-ladder side, but it
still does **not** recover the Hidayat Table 2 invariant at the reported
`51.4 at% O` wustite composition.

The next pragmatic Fe-side upgrade was to replace pure `Fe_bcc` with the
Hidayat `BCC_A2` Fe-O solution model:

$$
g^{\mathrm{bcc}}(T,x_{\mathrm O}) =
(1-x_{\mathrm O}) g_{\mathrm{Fe}}^{\circ}(T)
+ x_{\mathrm O} g_{\mathrm O}^{\circ}(T)
+ RT \left[(1-x_{\mathrm O})\ln(1-x_{\mathrm O}) + x_{\mathrm O}\ln x_{\mathrm O}\right]
+ x_{\mathrm O}(1-x_{\mathrm O}) L_{\mathrm{Fe,O}}(T)
$$

with:

$$
g_{\mathrm O}^{\circ}(T) =
120184.8 + 139.1406T - 24.5000T\ln T - 9.8420\times 10^{-4}T^2
- 0.12938\times 10^{-6}T^3 + \frac{322517}{T}
$$

$$
L_{\mathrm{Fe,O}}(T) = -315149.19 + 20.6935T
$$

and `g_{\mathrm{Fe}}^{\circ}(T)` taken from the bcc iron line plus the unary
Hillert-Jarl magnetic term already implemented.

This Fe-side upgrade turns out to have essentially **no effect** on the
Hidayat eutectoid validation:

- at the target point, the `BCC_A2 | wustite` residual is still about
  `-15.146 kJ/mol`,
- the grand-potential minimum occurs at an oxygen fraction
  `y_O \approx 10^{-8}` in the metallic phase,
- the best-fit `BCC_A2 | wustite` boundary remains at about
  `x = 0.362558` (`54.155 at% O`).

So the current evidence is that the remaining Tier 2 mismatch is **not**
caused by neglecting oxygen solubility in metallic iron. The dominant gap is
still elsewhere in the Fe-O stack.

To localize the remaining shift further, an additional condensed-only target
from Hidayat Table 2 / Fig. 12 was checked:

$$
\mathrm{Fe(fcc)} + \mathrm{wustite} \rightarrow \mathrm{Fe(bcc)}
\qquad \text{at } 912~^\circ\mathrm C
$$

with the reported wustite composition:

$$
51.3~\text{at\% O}
$$

For the current model:

- at the Hidayat target point `(T = 1185.15 K, x = 0.106776)`,
  the `Fe(bcc) | wustite` residual is about `-17.255 kJ/mol`,
- at the same point, the `Fe(fcc) | wustite` residual is about
  `-15.047 kJ/mol`,
- the direct Gibbs offset between `Fe(fcc)` and `Fe(bcc)` there is only about
  `+2.208 kJ/mol`,
- the best coupled `Fe(bcc)/Fe(fcc)/wustite` invariant fit moves to about
  `x = 0.315384` (`53.654 at% O`) with `max|residual| \approx 1.109 kJ/mol`.

This is an important diagnostic result:

- the `Fe | wustite` boundary family itself is shifted to higher oxygen
  contents by roughly `2.3 at% O`,
- that shift persists through both the low-temperature eutectoid and the
  `bcc/fcc` crossover,
- so the remaining mismatch is not a single isolated invariant-point error.

The next diagnostic step was to check the pure-metal crossover directly in the
current implementation:

$$
\mathrm{Fe(fcc)} \rightleftharpoons \mathrm{Fe(bcc)}.
$$

For the current pragmatic Hidayat subset, the pure-metal Gibbs functions cross
at about:

$$
T \approx 835.18~\mathrm K \approx 562.03~^\circ\mathrm C,
$$

whereas the Hidayat condensed-phase invariant involving wustite is reported at:

$$
T = 1185.15~\mathrm K = 912~^\circ\mathrm C.
$$

At `1185.15 K`, the current direct Gibbs offset is still:

$$
G_{\mathrm{Fe(fcc)}} - G_{\mathrm{Fe(bcc)}} \approx +2.208~\mathrm{kJ/mol}.
$$

This sharpens the diagnosis substantially:

- the remaining Tier 2 mismatch is already present in the pure `Fe_bcc/Fe_fcc`
  thermodynamic crossing,
- so it cannot be attributed only to wustite, spinel, or oxygen dissolved in
  metallic iron,
- the next refinement target is the exact iron unary/magnetic treatment rather
  than another oxide-side plumbing change.

The next correction was to replace the earlier direct `Fe_bcc/Fe_fcc`
implementation with the standard SGTE-style iron unary plus encoded magnetic
parameters:

$$
G_{\mathrm{bcc}} = GHSERFE + G_{\mathrm{mag}}(T; 1043,\ 2.22,\ 0.40)
$$

$$
G_{\mathrm{fcc}} = \left(GHSERFE - 1462.4 + 8.282T - 1.15T\ln T + 6.4\times10^{-4}T^2\right)
+ G_{\mathrm{mag}}(T; -201,\ -2.1,\ 0.28)
$$

where the negative `fcc` magnetic parameters are the standard AFM encoding of
the physical values reported by Hidayat (`T_N = 67 K`, `\beta = 0.70`).

With that change:

- the pure `Fe(fcc) = Fe(bcc)` crossover moves to about
  `1184.73 K = 911.58 ^\circ C`,
- the direct Gibbs offset at `912 ^\circ C` becomes essentially zero,
- so the earlier pure-iron allotropic error is effectively removed.

This is a useful cleanup, but it does **not** remove the Fe-wustite boundary
shift:

- at `912 ^\circ C`, the common `Fe(bcc)/Fe(fcc)|wustite` fit still sits near
  `53.58 at% O`,
- at `561 ^\circ C`, the `Fe(bcc)|wustite` fit still sits near `54.22 at% O`.

So after the iron-side correction, the dominant remaining Tier 2 mismatch is
now localized much more clearly to the oxide side, especially the wustite /
oxide-ladder reference consistency, rather than the metallic iron model.

## 14A. Wustite Audit

The next audit step was to re-check the accepted Hidayat wustite model against
the paper text itself.

Two points are now clear:

- the accepted polynomial family is indeed model (3),

$$
(1-x)\mathrm{FeO} + x\mathrm{FeO}_{3/2}
$$

  not the oxygen-basis polynomial models (4) or (5),
- so the current mismatch is **not** caused by choosing the wrong overall
  wustite model family.

The relevant Hidayat wording is:

$$
\text{``In the present study, the polynomial model with formula unit (3) of the solution was accepted.''}
$$

The remaining ambiguity was the exact excess-term interpretation.

In Section 2.1, Hidayat writes the polynomial-model Gibbs energy as

$$
g = X_A g_A^\circ + X_B g_B^\circ - TS_{\mathrm{config}} + X_A X_B L_{A,B}
$$

with

$$
L_{A,B} = \sum_{i,j \ge 0} q_{A,B}^{ij} X_A^i X_B^j
$$

and Table 1 reports for wustite only:

$$
q_{\mathrm{FeO},\mathrm{FeO}_{1.5}}^{00} = -59412.8
$$

$$
q_{\mathrm{FeO},\mathrm{FeO}_{1.5}}^{10} = 42676.8
$$

The earlier code in `wustite_hidayat.c` used the Redlich-Kister-style form

$$
g^{ex}_{\mathrm{current}} = X_A X_B \left[q^{00} + q^{10}(X_A - X_B)\right]
$$

which is equivalent to

$$
g^{ex}_{\mathrm{current}} = x(1-x)\left[q^{00} + q^{10}(1-2x)\right].
$$

That interpretation is not consistent with Eq. (8) as written.

If Eq. (8) is read literally, then the two most obvious polynomial
interpretations are:

$$
g^{ex}_{A} = X_A X_B \left(q^{00} + q^{10} X_A\right)
$$

or

$$
g^{ex}_{B} = X_A X_B \left(q^{00} + q^{10} X_B\right).
$$

Using the current standalone diagnostic, the effect on the
`\mathrm{Fe}|\mathrm{wustite}` boundary is large:

- current RK-style interpretation:
  best fit near `54.22 at% O` at `561 ^\circ C` and `53.58 at% O` at
  `912 ^\circ C`,
- literal polynomial with `q^{10} X_A`:
  poor fit at `561 ^\circ C`,
- literal polynomial with `q^{10} X_B`:
  best fit moves much closer to Hidayat, near `52.17 at% O` at
  `561 ^\circ C` and `52.05 at% O` at `912 ^\circ C`.

The paper's notation therefore confirms that the accepted excess term should be
read as:

$$
g^{ex}_{\mathrm{Hidayat}} = X_A X_B \left(q^{00} + q^{10} X_A\right)
$$

The implementation has been updated accordingly.

This correction materially improves the Hidayat-facing validation:

- at `561 ^\circ C`, the `Fe(bcc)|wustite` residual at the Hidayat target point
  drops to about `+0.406 kJ/mol`,
- the best `Fe(bcc)|wustite` fit moves to about `51.35 at% O`, essentially
  matching the reported `51.4 at% O`,
- at `912 ^\circ C`, the common `Fe(bcc)/Fe(fcc)|wustite` residual at the
  Hidayat target point drops to about `+0.645 kJ/mol`,
- the best common `Fe(bcc)/Fe(fcc)|wustite` fit moves to about `51.26 at% O`,
  close to the reported `51.3 at% O`.

So the dominant Fe-wustite boundary mismatch was indeed caused by the earlier
misinterpretation of the wustite excess term.

## 15. Degterov Spinel Upgrade

The next Tier 2 extension is to replace the stoichiometric surrogate
`Fe3O4` with the Fe-only slice of the Degterov spinel CEF model.

### 15.1. Reduced Fe-Only Spinel Model

For Fe-O only, Degterov's general spinel

$$
(A,B,E)[A,B,E,V]_2 O_4
$$

reduces to the Fe-only case

$$
(A,E)[A,E,V]_2 O_4
$$

with:

$$
A = \mathrm{Fe}^{2+}, \qquad E = \mathrm{Fe}^{3+}, \qquad V = \mathrm{Va}
$$

The Degterov Table II parameters needed for this Fe-only slice are:

$$
G_{AE}(T) = -1141701 + 1014.54T - 0.00814919T^2 - 174.832T\ln T + \frac{1445276}{T}
$$

$$
I_{AE}(T) = -31229 + 22.063T
$$

$$
V_E(T) = 29932 + 28.547T
$$

$$
\Delta_{AE} = 15781
$$

with magnetic parameters:

$$
T_C = 848\ \mathrm{K}, \qquad \beta = 44.54, \qquad p = 0.28
$$

The additional Table II parameters involving Zn do **not** enter this Fe-only
slice.

This phase still carries the essential spinel machinery:

- nonstoichiometry toward the oxidized side,
- internal cation distribution between tetrahedral and octahedral sites,
- compatibility with the Hillert-Jarl magnetic treatment.

### 15.2. Planned Optimizer Representation

To avoid nonlinear balance constraints in `eqm`, the Fe-only spinel phase will
be represented using site-species amounts directly:

- tetrahedral `Fe2+`
- tetrahedral `Fe3+`
- octahedral `Fe2+`
- octahedral `Fe3+`
- octahedral `Va`

For one phase amount `N`, define the site amounts:

$$
n_{t,A},\ n_{t,E},\ n_{o,A},\ n_{o,E},\ n_{o,V}
$$

with linear constraints:

$$
2(n_{t,A} + n_{t,E}) - n_{o,A} - n_{o,E} - n_{o,V} = 0
$$

$$
6 n_{t,A} + 5 n_{t,E} - 2 n_{o,A} - 3 n_{o,E} = 0
$$

These enforce:

- sublattice occupancy,
- electroneutrality.

Element totals remain linear:

$$
n_{\mathrm{Fe}} = n_{t,A} + n_{t,E} + n_{o,A} + n_{o,E}
$$

$$
n_{\mathrm{O}} = 4(n_{t,A} + n_{t,E})
$$

This is the key implementation advantage over optimizing directly in terms of
overall oxidation state and inversion variables.

### 15.3. Thermodynamic Model Used In Code

The phase Gibbs energy will be evaluated from:

$$
G = N \left(
\sum_i \sum_j Y_i' Y_j'' G_{ij}
- T S_{\mathrm{conf}}
\right)
+ G_{\mathrm{mag}}
$$

with:

$$
S_{\mathrm{conf}} =
-R \left(
\sum_i Y_i' \ln Y_i'
+ 2 \sum_j Y_j'' \ln Y_j''
\right)
$$

and with the Fe-only pseudocomponent energies reconstructed from Degterov's
Table II parameters:

$$
G_{AE}(T)
$$

$$
I_{AE}(T)
$$

$$
V_E(T)
$$

$$
\Delta_{AE}
$$

plus the symmetry relation:

$$
G_{EA} = G_{AE}
$$

The first implementation uses the Degterov/Sundman pure-magnetite magnetic
triplet:

$$
T_C = 848~\mathrm{K}, \qquad \beta = 44.54, \qquad p = 0.28
$$

as a phase-level correction.

### 15.4. Current Assumption

The initial Fe-only implementation will set:

$$
\Delta_{EAV} = 0
$$

This is a deliberate first-pass assumption motivated by Degterov's discussion
that a single vacancy parameter can be sufficient for the oxygen
nonstoichiometry of the `(A,E)[A,E,V]_2 O_4` spinel.

If later extraction from Table II shows that `\Delta_{EAV}` should be nonzero
for the Fe-only slice, that parameter can be introduced without changing the
optimizer structure.

### 1.4. Hillert-Jarl Magnetic Term

Degterov gives the magnetic Gibbs contribution as:

$$
G^{\mathrm{magn}} = f(\tau)\, R T \ln(\beta + 1)
$$

with:

$$
\tau = \frac{T}{T_c}
$$

and

$$
f(\tau) =
1 - \left(
\frac{79}{140\,p}\tau^{-1}
+ \frac{474}{497}\left(\frac{1}{p}-1\right)
\left(\frac{\tau^3}{6} + \frac{\tau^9}{135} + \frac{\tau^{15}}{600}\right)
\right)\Big/ A,
\qquad \tau \le 1
$$

$$
f(\tau) =
-\left(
\frac{\tau^{-5}}{10} + \frac{\tau^{-15}}{315} + \frac{\tau^{-25}}{1500}
\right)\Big/ A,
\qquad \tau > 1
$$

where

$$
A = \frac{518}{1125} + \frac{11692}{15975}\left(\frac{1}{p}-1\right)
$$

Degterov also gives the general CEF expression for the average magnetic moment:

$$
\beta =
\sum_i \sum_j Y_i' Y_j'' \beta_{ij}
+ \sum_i \sum_j \sum_k Y_i' Y_j' Y_k'' \beta_{ij:k}
+ \sum_i \sum_j \sum_k Y_k' Y_j'' Y_i \beta_{k:ij}
$$

For the present pragmatic Tier 2 implementation, we do **not** need the full
CEF magnetic machinery. For unary fixed-composition phases, the simplification
is:

$$
\beta = \text{constant}
$$

with `\beta` taken directly from Hidayat Table 1 for the phase of interest.

So for the current reduced Fe-O subset, the next implementation target is just:

$$
G(T) = G_{\mathrm{base}}(T) + G^{\mathrm{magn}}(T; T_c \text{ or } T_N, \beta, p)
$$

for:

- `Fe_bcc`
- `Fe_fcc`
- `Fe2O3`

This is a small thermo-layer extension.

It does **not** require:

- new equilibrium variables,
- new constraints,
- any change to the solution-phase minimization framework,
- the full magnetite CEF inherited from Degterov.

## 2. Why Wustite Is The Architectural Breakpoint

The current equilibrium engine minimizes total Gibbs energy over species mole
numbers:

$$
\min_{\mathbf{n}} G(\mathbf{n})
$$

subject to:

$$
\mathbf{A}\mathbf{n} = \mathbf{b}, \qquad \mathbf{n} \ge 0
$$

In the current implementation, condensed species are treated as fixed-activity
species with one standard chemical potential each:

$$
\mu_i = \mu_i^\circ(T)
$$

while gases use:

$$
\mu_i = \mu_i^\circ(T) + RT \ln\left(\frac{y_i P}{P^\circ}\right)
$$

That is enough for stoichiometric solids, but not for wustite.

Wustite is not a line compound `FeO`; it is a solid solution over a finite
oxygen-rich composition range. Therefore its Gibbs energy must depend on an
internal composition coordinate.

## 3. Chosen Wustite Representation

For the first implementation, wustite will be represented as a binary solution
between two endmembers:

$$
A = \mathrm{FeO}
$$

$$
B = \mathrm{FeO}_{3/2}
$$

The phase composition is parameterized by:

$$
\mathrm{Wus} \equiv (1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{3/2}
$$

with:

$$
x = \text{mole fraction of the FeO}_{3/2}\text{ endmember}
$$

This is a thermodynamic parameterization, not a literal molecular statement
about the crystal.

## 4. Optimizer Variables: Endmember Amounts, Not Explicit x

The crucial design choice is:

- do **not** introduce the wustite composition `x` as a primary optimizer
  variable in the first implementation,
- instead optimize over endmember amounts directly.

Define:

$$
n_A = \text{moles of FeO endmember in wustite}
$$

$$
n_B = \text{moles of FeO}_{3/2}\text{ endmember in wustite}
$$

Then:

$$
N_w = n_A + n_B
$$

and:

$$
x = \frac{n_B}{n_A + n_B}
$$

This gives three implementation advantages:

1. element balances remain linear,
2. the existing `A n = b` structure survives,
3. no explicit extra composition variable is needed.

## 5. Element Accounting For Wustite

The endmember stoichiometries are:

$$
\mathrm{FeO}: \qquad (\mathrm{Fe}=1,\ \mathrm{O}=1)
$$

$$
\mathrm{FeO}_{3/2}: \qquad (\mathrm{Fe}=1,\ \mathrm{O}=3/2)
$$

So the wustite contribution to the element totals is:

$$
n_{\mathrm{Fe},w} = n_A + n_B
$$

$$
n_{\mathrm{O},w} = n_A + \frac{3}{2}n_B
$$

Equivalently, in terms of `N_w` and `x`:

$$
n_{\mathrm{Fe},w} = N_w
$$

$$
n_{\mathrm{O},w} = N_w \left(1 + \frac{x}{2}\right)
$$

This is exactly the behavior we want.

## 6. Gibbs Energy Model For Wustite

Let:

$$
g_A^\circ(T) = \text{standard molar Gibbs energy of FeO endmember}
$$

$$
g_B^\circ(T) = \text{standard molar Gibbs energy of FeO}_{3/2}\text{ endmember}
$$

Let the excess Gibbs term be:

$$
g^{\mathrm{ex}}(T,x)
$$

Then the molar Gibbs energy of the binary solution phase is:

$$
g_w(T,x) =
(1-x)g_A^\circ(T)
+ x g_B^\circ(T)
+ RT \left[(1-x)\ln(1-x) + x\ln x\right]
+ g^{\mathrm{ex}}(T,x)
$$

and the total wustite Gibbs energy is:

$$
G_w = N_w\,g_w(T,x)
$$

For implementation in terms of endmember amounts, it is more convenient to use:

$$
G_w =
n_A g_A^\circ(T)
+ n_B g_B^\circ(T)
+ RT\left[n_A \ln\left(\frac{n_A}{N_w}\right)
+ n_B \ln\left(\frac{n_B}{N_w}\right)\right]
+ N_w g^{\mathrm{ex}}(T,x)
$$

with:

$$
N_w = n_A + n_B, \qquad x = \frac{n_B}{N_w}
$$

This form is the one the optimizer should evaluate directly.

### 6.1. Hidayat et al. (2015) Accepted Wustite Model

For the current implementation, the intended wustite model is the accepted
binary Bragg-Williams form reported by Hidayat et al. (2015), with:

$$
\mathrm{Wus} \equiv (1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{3/2}
$$

where:

$$
x = \text{mole fraction of the FeO}_{3/2}\text{ endmember}
$$

The endmember Gibbs energies are:

$$
g^\circ_{\mathrm{FeO}}(T) =
-285203.5
+ 274.2455\,T
- 49.19444\,T\ln T
- 0.004678477\,T^2
+ \frac{297568.8}{T}
+ 574.4469\,\ln T
$$

$$
g^\circ_{\mathrm{FeO}_{3/2}}(T) =
-523138.0
+ 73.37019\,T
- 26.96809\,T\ln T
- 0.008835071\,T^2
+ \frac{1498519}{T}
+ 25471.09\,\ln T
$$

The total molar Gibbs energy is:

$$
g_w(T,x) =
(1-x)\,g^\circ_{\mathrm{FeO}}(T)
+ x\,g^\circ_{\mathrm{FeO}_{3/2}}(T)
+ RT\left[(1-x)\ln(1-x) + x\ln x\right]
+ g^{\mathrm{ex}}(x)
$$

with the excess term:

$$
g^{\mathrm{ex}}(x) = x(1-x)\left[q_{00} + q_{10}(1-x)\right]
$$

and coefficients:

$$
q_{00} = -59412.8\ \mathrm{J/mol}
$$

$$
q_{10} = 42676.8\ \mathrm{J/mol}
$$

In terms of the Fe:O ratio implied by this endmember parameterization:

$$
\frac{O}{Fe} = 1 + \frac{x}{2}
$$

This follows directly from:

$$
n_{\mathrm{Fe},w} = N_w
$$

$$
n_{\mathrm{O},w} = N_w\left(1 + \frac{x}{2}\right)
$$

The Hidayat model itself does **not** impose any additional explicit
composition bounds beyond the mathematical requirement:

$$
0 < x < 1
$$

For the current implementation, positivity of the endmember amounts
`n_A > 0` and `n_B > 0` is therefore sufficient to keep `x` in the interior
of the model domain.

## 7. Chemical Potentials Needed By The Solver

The optimizer needs the endmember chemical potentials:

$$
\mu_A^w = \frac{\partial G_w}{\partial n_A}
$$

$$
\mu_B^w = \frac{\partial G_w}{\partial n_B}
$$

Equilibrium with element potentials is then:

$$
\mu_A^w = \lambda_{\mathrm{Fe}} + \lambda_{\mathrm{O}}
$$

$$
\mu_B^w = \lambda_{\mathrm{Fe}} + \frac{3}{2}\lambda_{\mathrm{O}}
$$

This plugs naturally into the existing elemental-balance formulation.

## 8. Composition Bounds As Linear Inequalities

For a generic binary solution, if one wants to impose explicit composition
bounds:

$$
x_{\min} \le x \le x_{\max}
$$

with:

$$
x = \frac{n_B}{n_A + n_B}
$$

These can be rewritten as linear inequalities.

Lower bound:

$$
\frac{n_B}{n_A+n_B} \ge x_{\min}
$$

which becomes:

$$
(1-x_{\min})n_B - x_{\min}n_A \ge 0
$$

Upper bound:

$$
\frac{n_B}{n_A+n_B} \le x_{\max}
$$

which becomes:

$$
x_{\max}n_A - (1-x_{\max})n_B \ge 0
$$

This is the key reason to prefer endmember amounts over an explicit `x`
variable in the first version.

For the accepted Hidayat wustite model, however, no extra explicit
composition-bounds are part of the model. So for the current implementation we
do not plan to add any additional inequality constraints for wustite beyond the
positivity of the endmember amounts:

$$
n_A > 0, \qquad n_B > 0
$$

which already implies:

$$
0 < x < 1
$$

## 9. Thermodynamic Data Strategy

The first implementation needs two distinct data layers:

### 9.1 Wustite

The accepted Hidayat model is now specified for implementation as:

$$
g_A^\circ(T) =
-285203.5
+ 274.2455\,T
- 49.19444\,T\ln T
- 0.004678477\,T^2
+ \frac{297568.8}{T}
+ 574.4469 \ln T
$$

$$
g_B^\circ(T) =
-523138.0
+ 73.37019\,T
- 26.96809\,T\ln T
- 0.008835071\,T^2
+ \frac{1498519}{T}
+ 25471.09 \ln T
$$

with excess Gibbs term:

$$
g^{\mathrm{ex}}(x) = x(1-x)\left[q_{00} + q_{10}(1-x)\right]
$$

where:

$$
q_{00} = -59412.8 \ \mathrm{J/mol}
$$

$$
q_{10} = 42676.8 \ \mathrm{J/mol}
$$

No temperature dependence is included in the accepted excess term.

For the current implementation, the thermodynamic domain is:

$$
0 < x < 1
$$

with:

$$
x = \frac{n_B}{n_A + n_B}
$$

and `x` interpreted as the mole fraction of the `FeO1.5` endmember.

### 9.2 Stoichiometric Fe-O Solids

We also need a self-consistent Fe-O dataset for:

- `Fe`
- `Fe3O4`
- `Fe2O3`

The existing placeholder entries in `constcp_data.c` are not sufficient for a
credible Tier 2 implementation.

## 10. Proposed Code Architecture

### 10.1 New binary-solution thermo model

Add a new model layer for condensed binary solutions.

Suggested files:

- `solution.h`
- `solution.c`
- `solution_data.h`
- `solution_data.c`
- `wustite_hidayat.h`
- `wustite_hidayat.c`

Suggested core model shape:

```c
typedef struct {
    double (*g0_a)(double T, double p, FpropsError *err);
    double (*g0_b)(double T, double p, FpropsError *err);
    double (*gex)(double T, double x, const void *params);
    double (*dgex_dx)(double T, double x, const void *params);
    double (*d2gex_dx2)(double T, double x, const void *params);
    const void *params;
    double xmin;
    double xmax;
} BinarySolutionModel;
```

The first implementation should support one registered binary phase:

- `wustite_hidayat`

### 10.2 Equilibrium system metadata

Extend the equilibrium data structures to distinguish:

- ideal-gas species,
- pure condensed species,
- solution-phase members.

Suggested additions to `eqm_internal.h`:

```c
typedef enum {
    EQM_SPEC_GAS,
    EQM_SPEC_PURE_COND,
    EQM_SPEC_SOLN_MEMBER
} EqmSpeciesKind;

typedef struct {
    int ia;
    int ib;
    const BinarySolutionModel *model;
} EqmBinaryPhase;
```

and the main working data should also track:

- per-species kind,
- per-species phase membership,
- a list of active binary phases.

### 10.3 Public representation in the first version

For the first version, keep the public equilibrium API unchanged.

Expose wustite internally as two pseudo-species:

- `WUS_A` or `Wus_FeO`
- `WUS_B` or `Wus_FeO1p5`

with the usual element matrix columns:

$$
\mathrm{WUS\_A}: (\mathrm{Fe}=1,\ \mathrm{O}=1)
$$

$$
\mathrm{WUS\_B}: (\mathrm{Fe}=1,\ \mathrm{O}=3/2)
$$

The system builder will recognize that these two names belong to one condensed
solution phase and evaluate them jointly.

This minimizes API churn while keeping the implementation correct.

## 11. Required Changes In `eqm`

### 11.1 Replace fixed condensed `mu0` handling with phase-aware evaluation

The current evaluator in `eqm.c` assumes each condensed entry contributes a
fixed chemical potential:

$$
\mu_i = \mu_i^\circ(T)
$$

That must become phase-aware:

- gas species keep the current ideal-gas treatment,
- pure condensed species keep the fixed-activity treatment,
- solution members are evaluated in groups.

For a binary solution phase, the evaluator must:

1. gather `n_A`, `n_B`,
2. compute `N_w`,
3. compute `x = n_B / N_w`,
4. compute `G_w`,
5. return `\mu_A^w` and `\mu_B^w`.

### 11.2 Linear inequality support

For a generic binary solution model, linear inequality support may still be
useful:

$$
\mathbf{A}\mathbf{n} = \mathbf{b}
$$

We need to support:

$$
\mathbf{A}\mathbf{n} = \mathbf{b}
$$

$$
\mathbf{C}\mathbf{n} \ge \mathbf{d}
$$

where `C` and `d` encode phase-composition bounds.

For the current Hidayat wustite implementation, this is not required.

## 12. Solver Strategy

### 12.1 First solver target: IPOPT in n-space

The simplest first implementation path is:

- extend the IPOPT `n`-space solver,
- evaluate objective and gradients in full species space.

This avoids the extra complexity of reduced-space/nullspace handling with
solution phases.

### 12.2 Defer nullspace/reduced support

The nullspace machinery is valuable for the current pure-equality ideal-gas
systems, but it is not the right place to start for wustite.

When any solution phase is present:

- disable reduced/nullspace pathways,
- route `auto` to IPOPT `n`-space first,
- use SLSQP only as a fallback after inequality support is added there.

### 12.3 Hessian policy

For the first implementation, prioritize:

- correct objective evaluation,
- correct gradients,
- robust convergence.

Exact Hessians can wait.
Using a quasi-Newton / limited-memory Hessian strategy is acceptable initially.

## 13. Phased Implementation Plan

### Phase 1: Scaffolding

1. Add `FEO.md` and keep it current.
2. Add binary-solution model interfaces and a placeholder implementation.
3. Add equilibrium metadata for solution-phase membership.
4. Add linear inequality support in the IPOPT `n`-space path.
5. Route solution-phase problems away from reduced/nullspace solvers.

### Phase 2: Wustite thermodynamics

1. Implement the Hidayat wustite model exactly.
2. Register the two wustite endmembers.
3. Rely on endmember positivity to keep the model in the interior domain.
4. Add unit tests for:
   - `g_w(T,x)`
   - `mu_A`
   - `mu_B`
   - finite-difference derivative checks

### Phase 3: Fe-O stoichiometric phases

1. Replace placeholder `Fe`, `Fe3O4`, `Fe2O3` data with self-consistent values.
2. Confirm relative phase stability and oxygen potential ordering.

### Phase 4: Fe-O validation

1. Reproduce Fe | wustite boundaries.
2. Reproduce wustite | magnetite boundaries.
3. Check the disappearance of stable wustite below its stability limit.
4. Compare selected results against Reaktoro / published Fe-O boundaries.

### Phase 5: Tier 3 readiness

Once Tier 2 is stable, add gas species for Fe-O-H:

- `H2`
- `H2O`
- optionally `O2`

and validate hydrogen reduction boundaries before moving to Fe-O-C-H.

## 14. Validation Matrix

Minimum planned validation cases:

### 14.1 Unit-level

- evaluate `g_w(T,x)` at representative `T` and `x`,
- verify `mu_A` and `mu_B` by finite differences,
- verify that `x`-bounds are respected through inequality constraints,
- verify correct behavior as `x` approaches bounds.

### 14.2 System-level Fe-O

- `Fe + WUS_A + WUS_B`
- `Fe + WUS_A + WUS_B + Fe3O4`
- `Fe3O4 + WUS_A + WUS_B + Fe2O3`
- full `Fe + WUS_A + WUS_B + Fe3O4 + Fe2O3`

### 14.3 Diagnostic outputs

For Fe-O runs, record:

- species amounts,
- inferred wustite composition:

$$
x = \frac{n_B}{n_A+n_B}
$$

- phase Gibbs contributions,
- elemental residuals,
- KKT / stationarity diagnostics if available.

## 15. Open Data Items

Before we can populate the final thermodynamic model, we still need from the
Hidayat reassessment:

1. the matching Fe / Fe3O4 / Fe2O3 data from the same assessment.

## 16. Immediate Next Step

The next coding step should be:

1. add the binary-solution scaffolding,
2. extend the equilibrium data structures,
3. extend the IPOPT `n`-space solver to support linear inequalities,
4. wire in a placeholder binary solution before populating Hidayat data.

That lets us verify the architecture before committing to the final
thermodynamic coefficients.

## 17. Tier 3 Plan: Fe-O-H

Tier 3 should now proceed on top of the current Fe-O backbone, with the
important caveat that the `\mathrm{Fe}|\mathrm{wustite}` hydrogen boundaries are
expected to be much more trustworthy than the more oxygen-rich
`\mathrm{wustite}|\mathrm{spinel}` side.

### 17.1 Scope

The Tier 3 target is:

$$
\mathrm{Fe} - \mathrm{O} - \mathrm{H}
$$

with:

- ideal gas species:
  - `H2`
  - `H2O`
  - optionally `O2` for direct oxygen-potential diagnostics
- condensed phases:
  - `Fe_bcc`
  - `Fe_fcc`
  - wustite
  - spinel
  - `Fe2O3`

No new condensed-solution framework is required for Tier 3. The current
full-space equilibrium machinery already supports the needed phase set.

### 17.2 Immediate implementation tasks

1. Build a dedicated Tier 3 validation script alongside the current Fe-O
   diagnostic.
2. Add end-to-end `eqm` tests for hydrogen reduction with:
   - `Fe_bcc`, `Wus_FeO`, `Wus_FeO1p5`, `hydrogen`, `water`
   - `Fe_fcc`, `Wus_FeO`, `Wus_FeO1p5`, `hydrogen`, `water`
   - `Wus_FeO`, `Wus_FeO1p5`, spinel members, `hydrogen`, `water`
3. Add reporting helpers for:
   - gas ratio

$$
\frac{p_{\mathrm{H_2O}}}{p_{\mathrm{H_2}}}
$$

   - inferred wustite composition

$$
x = \frac{n_B}{n_A + n_B}
$$

   - dominant condensed assemblage
4. Add scan-based diagnostics versus temperature and gas composition.

### 17.3 First validation targets

The first Tier 3 targets should be the hydrogen analogues of the Fe-O
boundaries already audited in Tier 2.

#### A. Fe | wustite hydrogen boundary

Use the equilibrium condition:

$$
\mathrm{FeO} + \mathrm{H_2} \rightleftharpoons \mathrm{Fe} + \mathrm{H_2O}
$$

as the first primary diagnostic, generalized to nonstoichiometric wustite
through the element-potential solution already used in the Fe-O audit.

This should become the first trustworthy Tier 3 output because:

- the metallic iron side is now consistent,
- the accepted wustite model is now interpreted correctly,
- this boundary is the most directly relevant reduction threshold for DRI.

#### B. Wustite | spinel hydrogen-side boundary

Then test the gas compositions at which the stable condensed assemblage changes
between wustite-bearing and spinel-bearing states.

This should be treated as provisional until the oxide-ladder side is tightened
further.

### 17.4 Recommended implementation order

1. Add a `Fe + wustite + H2 + H2O` regression at one temperature.
2. Add a temperature sweep for the corresponding equilibrium
   `p_{\mathrm{H_2O}} / p_{\mathrm{H_2}}` ratio.
3. Compare those results against Reaktoro and/or classic reduction-boundary
   literature.
4. Only after that, add `spinel` and `Fe2O3` into the hydrogen-bearing scans.
5. Once those are stable, move to Tier 4 with `CO`, `CO2`, and WGS coupling.

### 17.5 What success looks like

Tier 3 is in good enough shape to proceed if:

- `Fe|wustite` hydrogen boundaries are smooth in temperature,
- the predicted `\mathrm{H_2O}/\mathrm{H_2}` threshold is stable under solver
  restarts and species reordering,
- the condensed assemblage transitions are physically ordered:

$$
\mathrm{Fe} \leftrightarrow \mathrm{wustite} \leftrightarrow \mathrm{spinel}
\leftrightarrow \mathrm{Fe_2O_3}
$$

- and the remaining disagreement is clearly localized to the oxide-rich side,
  not to the gas treatment or the wustite/metal boundary.

### 17.6 First implemented Tier 3 baseline

The first Tier 3 pass is now implemented in two pieces:

1. A mixed-source C regression using:

$$
\text{Fe-O condensed phases from Hidayat 2015} + \text{H}_2/\text{H}_2\text{O gas } \mu^\circ(T)
\text{ from } \texttt{reaktoro\_clone\_supcrt98}
$$

2. A standalone diagnostic script:

`models/johnpye/fprops/test/feoh_hydrogen_boundary.py`

which prints a first Baur-Glaessner-style table for the
`\mathrm{Fe}|\mathrm{wustite}` hydrogen boundary.

With the current implementation, the first-pass table is:

| T (°C) | Metal | x_wustite | at% O | log10(H2O/H2) | log10(H2/H2O) |
|---:|:---:|---:|---:|---:|---:|
| 600 | bcc | 0.109190 | 51.329 | -2.405616 | 2.405616 |
| 700 | bcc | 0.105793 | 51.288 | -2.163521 | 2.163521 |
| 800 | bcc | 0.104172 | 51.269 | -1.979391 | 1.979391 |
| 900 | bcc | 0.102956 | 51.255 | -1.842411 | 1.842411 |
| 1000 | fcc | 0.102302 | 51.247 | -1.739162 | 1.739162 |
| 1100 | fcc | 0.101429 | 51.237 | -1.667866 | 1.667866 |
| 1200 | fcc | 0.100136 | 51.221 | -1.626218 | 1.626218 |

This is not yet final validation, but it is a coherent Tier 3 starting point:

- the boundary is smooth in temperature,
- the iron allotropic switch appears near the expected range,
- the wustite composition along the boundary stays near `51.2-51.3 at% O`,
- and the gas threshold moves monotonically in the expected direction.

### 17.7 Provisional wustite-spinel hydrogen boundary

The same Tier 3 diagnostic script now also prints a provisional
`\mathrm{wustite}|\mathrm{spinel}` hydrogen-side boundary:

| T (°C) | Phase | x_wustite | at% O | log10(H2O/H2) | log10(H2/H2O) |
|---:|:---:|---:|---:|---:|---:|
| 600 | spinel | 0.108536 | 51.321 | -2.409596 | 2.409596 |
| 700 | spinel | 0.156632 | 51.884 | -1.871614 | 1.871614 |
| 800 | spinel | 0.198392 | 52.363 | -1.443879 | 1.443879 |
| 900 | spinel | 0.233265 | 52.755 | -1.098202 | 1.098202 |
| 1000 | spinel | 0.263495 | 53.090 | -0.812247 | 0.812247 |
| 1100 | spinel | 0.290021 | 53.380 | -0.576027 | 0.576027 |
| 1200 | spinel | 0.314232 | 53.642 | -0.379137 | 0.379137 |

This table is useful for trend-checking only. It should still be treated as
provisional because the oxide-rich side remains the least-certain part of the
current Tier 3 backbone.

### 17.8 First direct Reaktoro checkpoint

A first direct external comparison is now possible, but only on the oxide side.

Current local Reaktoro database availability is:

- `SupcrtDatabase("supcrt98")`:
  - usable at high temperature for `Ferrous-Oxide`, `Magnetite`, `H2(g)`,
    `H2O(g)`
  - does **not** expose metallic Fe as a solid species here
- `PhreeqcDatabase("llnl.dat")`:
  - exposes `Fe`, `FeO`, `Magnetite`, `H2(g)`, `H2O(g)`
  - but returns non-finite values above about `350 ^\circ C`

So the only practical high-temperature Reaktoro comparison currently available
in this environment is:

$$
\text{FPROPS } \mathrm{wustite}|\mathrm{spinel}
\quad \text{vs} \quad
\text{Reaktoro } \mathrm{FeO}|\mathrm{Magnetite}
$$

using `SupcrtDatabase("supcrt98")` on the Reaktoro side.

The first-pass comparison gives:

| T (°C) | FPROPS log10(H2O/H2) | Reaktoro log10(H2O/H2) | Delta |
|---:|---:|---:|---:|
| 600 | -2.410430 | 0.811619 | -3.222049 |
| 700 | -1.871514 | 0.990564 | -2.862078 |
| 800 | -1.443143 | 1.112621 | -2.555764 |
| 900 | -1.097676 | 1.194714 | -2.292390 |
| 1000 | -0.812558 | 1.247088 | -2.059646 |
| 1100 | -0.576692 | 1.276875 | -1.853567 |
| 1200 | -0.377662 | 1.289176 | -1.666838 |

That comparison should not be over-interpreted, because it mixes:

- nonstoichiometric wustite plus reduced spinel in FPROPS
- stoichiometric `FeO` plus stoichiometric `Magnetite` in Reaktoro

Still, it is useful as a first external signal: the oxide-side Tier 3 boundary
is still much more reducing in the current FPROPS model than in that
stoichiometric Reaktoro surrogate.

### 17.9 Gas-only audit: H2 + 1/2 O2 = H2O

To separate gas-source effects from condensed Fe-O thermodynamics, a dedicated
audit script has been added:

`models/johnpye/fprops/test/h2_oxidation_gas_audit.py`

It compares the standard-state reaction equilibrium for

$$
\mathrm{H_2}(g) + \frac12 \mathrm{O_2}(g) \rightleftharpoons \mathrm{H_2O}(g)
$$

across three sources:

- FPROPS `reaktoro_clone_supcrt98`
- FPROPS `Moran and Shapiro`
- direct Reaktoro `SupcrtDatabase("supcrt98")`

The first-pass comparison is:

| T (°C) | log10K clone | log10K M&S | log10K Reaktoro | M&S - clone | clone - Reaktoro |
|---:|---:|---:|---:|---:|---:|
| 600 | 11.821900 | 12.054152 | 11.820942 | 0.232252 | 0.000958 |
| 700 | 10.261533 | 10.551732 | 10.261648 | 0.290199 | -0.000115 |
| 800 | 8.973711 | 9.327238 | 8.974558 | 0.353527 | -0.000847 |
| 900 | 7.888994 | 8.309777 | 7.889600 | 0.420782 | -0.000605 |
| 1000 | 6.959147 | 7.450700 | 6.958789 | 0.491553 | 0.000358 |

The conclusions are:

- the FPROPS `reaktoro_clone_supcrt98` gas source tracks direct Reaktoro
  essentially exactly over this range;
- the `Moran and Shapiro` gas source is systematically more oxidizing for the
  water-formation equilibrium by about `0.23-0.49` log-units in `log10 K`
  between `600` and `1000 ^\circ C`.

This is important for the Tier 3 validation story because it bounds how much of
the current Baur-Glaessner `\mathrm{Fe}|\mathrm{wustite}` discrepancy can be
explained by gas thermodynamics alone. The present mismatch against the fitted
H2 line is about `1.36-2.35` log-units in `log10(H2O/H2)` over `600-1000
^\circ C`, so gas-source choice is clearly part of the difference, but not the
whole difference.

### 17.10 Baur-Glaessner fit comparison harness

A reusable comparison script has now been added:

`models/johnpye/fprops/test/feoh_baur_glaessner_compare.py`

It evaluates the current FPROPS Tier 3 boundary and a fitted
Baur-Glaessner-style polynomial on the same assumed GOD basis,

$$
\mathrm{GOD} = \frac{p(\mathrm{H_2O})}{p(\mathrm{H_2}) + p(\mathrm{H_2O})},
$$

and reports:

- `GOD_fit(T)` from the supplied polynomial,
- `GOD_model(T)` from the current FPROPS boundary,
- `\log_{10}(H_2O/H_2)` for both,
- residuals in both GOD and `\log_{10}(H_2O/H_2)`.

The script currently includes presets for:

- `h2-fe-wustite`
- `h2-wustite-spinel`

The first run against the supplied H2 `\mathrm{Fe}|\mathrm{wustite}` fit gives:

| T (°C) | GOD fit | GOD model | Delta GOD | log10 fit | log10 model | Delta log10 |
|---:|---:|---:|---:|---:|---:|---:|
| 600 | 0.470724 | 0.003915 | -0.466809 | -0.050916 | -2.405616 | -2.354700 |
| 700 | 0.409937 | 0.006816 | -0.403122 | -0.158181 | -2.163521 | -2.005341 |
| 800 | 0.362454 | 0.010377 | -0.352077 | -0.245258 | -1.979391 | -1.734133 |
| 900 | 0.324666 | 0.014171 | -0.310495 | -0.318082 | -1.842411 | -1.524329 |

Over those in-range points, the RMS discrepancy is about `1.93` in
`\Delta \log_{10}(H_2O/H_2)`, which confirms that the current mismatch to this
published fit is substantial rather than marginal.

### 17.11 Simplified stoichiometric FeO diagnostic

A stripped-down comparison script has been added:

`models/johnpye/fprops/test/feoh_stoich_feo_compare.py`

It compares:

- the current `\mathrm{Fe}|\mathrm{wustite}` Tier 3 boundary, and
- a deliberately simplified stoichiometric
  `\mathrm{Fe}|\mathrm{FeO}` boundary built from the placeholder
  `constcp:ellingham_placeholder` condensed species.

This is a diagnostic only. The purpose is to see whether removing wustite
nonstoichiometry moves the hydrogen reduction line in a useful direction.

The first result is:

| T (°C) | log10 current | log10 constcp Fe/FeO | Delta |
|---:|---:|---:|---:|
| 600 | -2.405616 | 15.641435 | 18.047051 |
| 700 | -2.163521 | 14.322556 | 16.486077 |
| 800 | -1.979391 | 13.238663 | 15.218054 |
| 900 | -1.842411 | 12.329283 | 14.171694 |
| 1000 | -1.739162 | 11.552503 | 13.291665 |

This is not a subtle shift; it is a complete blow-up. The simplified
stoichiometric line lands at effectively `\mathrm{GOD} \approx 1` across the
whole range.

So the conclusion is not that "stoichiometric FeO works better." The conclusion
is that the current placeholder constant-`c_p` Fe/FeO data are not physically
usable for Tier 3 reduction-boundary work. They are not a plausible surrogate
for validating the Baur-Glaessner discrepancy.

### 17.12 Stoichiometric Fe|Fe3O4 line against the Spreitzer reference

The Tier 3 hydrogen-boundary script now also includes a cheap stoichiometric
`\mathrm{Fe}|\mathrm{Fe_3O_4}` line using:

- stable metallic Fe from the current Hidayat iron model, and
- stoichiometric `\mathrm{Fe_3O_4}` from the current Hidayat surrogate

with

$$
\lambda_O = \frac{g_{\mathrm{Fe_3O_4}} - 3 g_{\mathrm{Fe}}}{4}.
$$

This line has also been wired into the Spreitzer comparison harness as the
current-model counterpart to the published `\mathrm{Fe}|\mathrm{magnetite}`
reference line.

The first comparison gives:

| T (°C) | GOD fit | GOD model | log10 fit | log10 model | Delta log10 |
|---:|---:|---:|---:|---:|---:|
| 320 | 0.494418 | 0.000292 | -0.009697 | -3.534654 | -3.524956 |
| 400 | 0.494530 | 0.001041 | -0.009502 | -2.982203 | -2.972701 |
| 500 | 0.494671 | 0.003336 | -0.009259 | -2.475303 | -2.466044 |
| 560 | 0.494755 | 0.005709 | -0.009112 | -2.240985 | -2.231873 |

So even this simplified stoichiometric `\mathrm{Fe}|\mathrm{Fe_3O_4}` line is
still far more reducing than the Spreitzer reference line. That means the
large discrepancy is not caused only by the wustite/spinel machinery.

## 18. Next External Validation Targets

The current comparison against the trusted Spreitzer and Schenk (2019) H2
reference lines shows that the Tier 3 hydrogen boundaries are still far too
reducing. Since the internal Fe|wustite boundary construction is now
self-consistent, the next external data should be chosen to isolate whether the
remaining problem is:

- condensed Fe-O oxygen-potential scale,
- gas-side standard-state scale, or
- basis alignment between the two.

The most useful next data sources are:

### 18.1 O'Neill 1988 EMF Fe-O boundaries

This is the best next condensed-side anchor:

- H. St. C. O'Neill (1988),
  "Systems Fe-O and Cu-O: thermodynamic data for the equilibria
  Fe-'FeO', Fe-Fe3O4, 'FeO'-Fe3O4, Fe3O4-Fe2O3, Cu-Cu2O, and Cu2O-CuO from emf
  measurements"

Links:

- `https://msaweb.org/AmMin/AM73/AM73_470.pdf`
- `https://research.monash.edu/en/publications/systems-fe-o-and-cu-o-thermodynamic-data-for-the-equilibria-fe-fe`

Why it matters:

- it directly constrains the Fe-O oxygen potentials for the key condensed
  boundaries:
  - `Fe | "FeO"`
  - `Fe | Fe3O4`
  - `"FeO" | Fe3O4`
  - `Fe3O4 | Fe2O3`
- it allows us to test the condensed Fe-O ladder before involving `H2/H2O`
  gas equilibria.

If the current Fe-O model reproduces those oxygen potentials reasonably well,
then the remaining Spreitzer mismatch is much more likely to be on the gas/basis
alignment side.

### 18.2 Gas-only H2/H2O/O2 benchmark on a FactSage-like basis

The next best gas-side discriminator is a table or curve for:

$$
\mathrm{H_2} + \frac12 \mathrm{O_2} \rightleftharpoons \mathrm{H_2O}
$$

from a FactSage-like source over roughly `300-1000 ^\circ C`.

We already know:

- `reaktoro_clone_supcrt98` matches direct Reaktoro closely,
- `Moran and Shapiro` shifts the gas-only equilibrium by only about
  `0.23-0.49` log-units in `log10 K` over `600-1000 ^\circ C`.

That is not enough to explain the full Spreitzer mismatch, so a FactSage-side
gas benchmark would be highly diagnostic.

### 18.3 Direct Fe-wustite H2/H2O equilibrium data

For the hydrogen side, a useful classical anchor is:

- Turkdogan, McKewan, and Zwell (1965),
  "Rate of Oxidation of Iron to Wustite in Water-Hydrogen Gas Mixtures"

Link:

- `https://doi.org/10.1021/j100885a052`

Why it matters:

- it is directly about the `Fe / wustite / H2 / H2O` equilibrium problem,
- even a few equilibrium points would help separate a gas/basis problem from a
  condensed Fe-O problem.

### 18.4 Priority order

The recommended order for chasing external validation data is:

1. O'Neill 1988 EMF Fe-O oxygen-potential data
2. FactSage-like `H2/H2O/O2` gas benchmark
3. Direct `Fe | wustite` `H2/H2O` equilibrium data such as Turkdogan et al.

## 19. Moran and Shapiro Gas Data Audit

The `Moran and Shapiro` gas data in the tree were audited against the original
table image:

- `models/johnpye/fprops/test/moran-cp-polynomials.png`

The tabulated form is clearly:

$$
\frac{\bar c_p}{\bar R} = a + bT + cT^2 + dT^3 + eT^4
$$

with the scaled columns `b x 10^3`, `c x 10^6`, etc.

### 19.1 Data-entry/configuration bug found and fixed

Two problems were found in the `*_ms.c` files:

1. `cp0star` had been set to `1` instead of the species gas constant.
2. the default reference-state selector had been left as `FPROPS_REF_IIR`,
   whereas these ideal-gas formation-reference datasets should default to
   `FPROPS_REF_REF0`.

Those fixes were applied to:

- `fluids/hydrogen_ms.c`
- `fluids/oxygen_ms.c`
- `fluids/water_ms.c`
- `fluids/carbondioxide_ms.c`
- `fluids/carbonmonoxide_ms.c`

### 19.2 Pure-property check after the fix

Using:

- `models/johnpye/fprops/test/pureprops_compare`

the corrected M&S `c_p^0` values are now physically sensible and close to the
Helmholtz gas values. At `1000 ^\circ C`, `1 bar`:

| Species | Helmholtz cp0 (J/mol/K) | M&S cp0 (J/mol/K) |
|---|---:|---:|
| H2O | 44.6348 | 44.8395 |
| H2 | 31.2978 | 30.6010 |
| O2 | 35.9200 | 36.3883 |

So the previous collapse of all M&S `c_p^0` values toward about `8.3
\mathrm{J\,mol^{-1}\,K^{-1}}` was a real implementation error and is now fixed.

### 19.3 Gas-only water-formation audit after the fix

Re-running the gas-only benchmark for

$$
\mathrm{H_2} + \frac12 \mathrm{O_2} \rightleftharpoons \mathrm{H_2O}
$$

gives:

### 19.4 Native Helmholtz `REF0` chemistry anchors

The native Helmholtz species were extended to carry an explicit `ref0`
chemistry anchor, using Moran and Shapiro Global 9th edition Table A-25 data
at `298.15 K`, `1 atm`:

- elemental gases such as `H2`, `O2`, `N2` use `\Delta H_f^\circ = 0`
  together with tabulated absolute `S^\circ`
- species such as `H2O(g)`, `CO`, `CO2`, `NH3`, `CH4`, `C2H6`, `C3H8`,
  `C4H10`, and `C2H5OH` use tabulated `\Delta H_f^\circ` and `S^\circ`

This was implemented as `ref0` metadata on the native Helmholtz fluids, while
leaving the ordinary default FPROPS reference state unchanged.

The equilibrium-side selector now accepts:

$$
\texttt{helmholtz+ref0:}
$$

to request the chemistry-compatible Helmholtz basis explicitly.

The current validation result is:

- absolute species values `g(T,p)` and `\mu^\circ(T)` do not yet match the
  Moran and Shapiro ideal-gas basis species-by-species
- but the reaction Gibbs energy for

$$
\mathrm{H_2} + \frac12 \mathrm{O_2} \rightleftharpoons \mathrm{H_2O}
$$

does match closely on the `helmholtz+ref0` basis

At `600^\circ C`:

- Helmholtz `REF0`: `\Delta G^\circ \approx -199590\ \mathrm{J/mol}`
- Moran and Shapiro: `\Delta G^\circ \approx -199555\ \mathrm{J/mol}`

At `900^\circ C`:

- Helmholtz `REF0`: `\Delta G^\circ \approx -182991\ \mathrm{J/mol}`
- Moran and Shapiro: `\Delta G^\circ \approx -182943\ \mathrm{J/mol}`

So the chemistry-anchor work is now good enough for reaction-level gas
equilibrium auditing, but not yet a full species-by-species absolute-reference
alignment.

| T (°C) | log10K clone | log10K M&S | log10K Reaktoro | M&S - clone |
|---:|---:|---:|---:|---:|
| 600 | 11.821900 | 11.934901 | 11.820942 | 0.113000 |
| 700 | 10.261533 | 10.414406 | 10.261648 | 0.152873 |
| 800 | 8.973711 | 9.173959 | 8.974558 | 0.200248 |
| 900 | 7.888994 | 8.142541 | 7.889600 | 0.253546 |
| 1000 | 6.959147 | 7.271393 | 6.958789 | 0.312246 |

So after fixing the M&S implementation, the gas-only difference relative to the
Reaktoro/SUPCRT98-clone source is only about `0.11-0.31` log-units over
`600-1000 ^\circ C`.

### 19.4 Tier 3 consequence

The corrected M&S source does **not** bring the hydrogen reduction boundary
closer to the trusted Spreitzer and Schenk `\mathrm{Fe}|\mathrm{wustite}` line.
It actually moves it farther away.

The updated comparison gives:

| T (°C) | log10 fit | log10 model (M&S) | Delta |
|---:|---:|---:|---:|
| 600 | -0.050916 | -5.819487 | -5.768571 |
| 700 | -0.158181 | -5.725323 | -5.567143 |
| 800 | -0.245258 | -5.646379 | -5.401121 |
| 900 | -0.318082 | -5.582350 | -5.264269 |

So the earlier M&S bug was real and needed fixing, but it was **not** the main
cause of the Spreitzer mismatch.

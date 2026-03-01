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
g^{\mathrm{ex}}(x) = x(1-x)\left[q_{00} + q_{10}(1 - 2x)\right]
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
g^{\mathrm{ex}}(x) = x(1-x)\left[q_{00} + q_{10}(1-2x)\right]
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

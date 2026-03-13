# FPROPS Chemical Equilibrium: Thermodynamics and Algorithms

This note explains the current ideal-gas equilibrium implementation in `fprops` at a second-year thermo level, while also showing the numerical methods used in code.

Document roadmap:

- Part A gives common thermodynamic and optimization principles (independent of solver choice).
- Part B gives implementation pathways, with reduced-space first (primary) and full-space interior-point as a secondary path.
- Part C gives validation/diagnostics.
- Part D maps equations to code.
- Appendix A gives the secondary full-space interior-point pathway.

## Quick Start

This section is the practical front-end to the material below.
The rest of the document keeps the bottom-up thermodynamic and
numerical explanation.

### Public entry points

The public equilibrium API is in
[eqm.h](/home/john/ascend/models/johnpye/fprops/eqm.h):

- `eqm_solve(...)`
- `eqm_solve_elements(...)`
- `eqm_mu0_source(...)`

For most users, the two important calls are:

1. `eqm_mu0_source(...)`
   Use this when you want standard chemical potentials $\mu_i^\circ(T, P^\circ)$ for chosen species and data sources.

2. `eqm_solve_elements(...)`
   Use this when you want equilibrium composition from:
   - a species list
   - an element list
   - element totals `b`
   - a source/model specification
   - `T`, `P`

### Typical usage pattern

At a high level:

1. Choose the candidate species.
2. Choose the conserved elements.
3. Set the elemental totals `b`.
4. Choose a thermo source or source map.
5. Call `eqm_solve_elements(...)`.
6. Read the equilibrium mole amounts `n_out`.

### Source selection syntax

`eqm_mu0_source(...)` and `eqm_solve_elements(...)` accept:

- a simple source name, for example:
  - `Moran and Shapiro`
  - `hidayat_2015`
  - `degterov_2001`
- an explicit model selector, for example:
  - `ideal:Moran and Shapiro`
  - `helmholtz+ref0:`
  - `constcp:oecd_nea_tdb_vol6_nickel`
  - `shomate:reaktoro_clone_supcrt98`
- a per-species source map, for example:

```text
Ni=oecd_nea_tdb_vol6_nickel;NiO=oecd_nea_tdb_vol6_nickel;*=Moran and Shapiro
```

The wildcard `*=` gives a default for species not named explicitly.

This is the main mechanism for mixed-source equilibrium problems,
for example:

- condensed Fe-O data from `hidayat_2015`
- gas `H2/H2O/O2` from `helmholtz+ref0:`

### Recommended source choices

For current Fe-O-H work:

- gases:
  - `helmholtz+ref0:`
- Fe-O condensed backbone:
  - `hidayat_2015`
- reduced spinel model:
  - `degterov_2001`

### Runners and utilities

The simplest existing command-line helpers are:

- [eqm_mu0_runner.c](/home/john/ascend/models/johnpye/fprops/test/eqm_mu0_runner.c)
- [eqm_case_runner.c](/home/john/ascend/models/johnpye/fprops/test/eqm_case_runner.c)

Build them with:

```bash
scons models/johnpye/fprops/test/eqm_mu0_runner -j4
scons models/johnpye/fprops/test/eqm_case_runner -j4
```

Examples:

```bash
./models/johnpye/fprops/test/eqm_mu0_runner "helmholtz+ref0:" 1173.15 101325 \
  hydrogen oxygen water
```

```bash
./models/johnpye/fprops/test/eqm_case_runner wgs 1173.15 101325 auto_nullspace "helmholtz+ref0:"
```

### A concrete Fe-O-H example: H2 reduction of hematite

Suppose we want a first-principles Fe-O-H equilibrium problem for
hematite reduced by hydrogen, allowing the system to choose among:

- hematite
- spinel
- wustite
- metallic iron
- `H2`
- `H2O`

One practical species list is:

```c
static const char *names[] = {
    "Fe_bcc", "Fe_fcc",
    "Wus_FeO", "Wus_FeO1p5",
    "Sp_Fe2_tet", "Sp_Fe3_tet", "Sp_Fe2_oct", "Sp_Fe3_oct", "Sp_Va_oct",
    "Fe2O3",
    "hydrogen", "water"
};

static const char *elements[] = {"Fe", "O", "H"};
```

If the feed is `1 mol Fe2O3 + 3 mol H2`, then the elemental totals are:

$$
b_{\mathrm{Fe}} = 2,\qquad
b_{\mathrm{O}} = 3,\qquad
b_{\mathrm{H}} = 6
$$

so:

```c
static const double b[] = {2.0, 3.0, 6.0};
```

A suitable mixed-source map is:

```text
Fe_bcc=hidayat_2015;Fe_fcc=hidayat_2015;
Wus_FeO=hidayat_2015;Wus_FeO1p5=hidayat_2015;
Fe2O3=hidayat_2015;
Sp_Fe2_tet=degterov_2001;Sp_Fe3_tet=degterov_2001;
Sp_Fe2_oct=degterov_2001;Sp_Fe3_oct=degterov_2001;Sp_Va_oct=degterov_2001;
hydrogen=helmholtz+ref0:;water=helmholtz+ref0:
```

Then solve:

```c
double n_out[12];
int status = eqm_solve_elements(
    names, 12,
    elements, 3,
    b,
    source_map,
    1173.15,      /* T = 900 C */
    101325.0,     /* P = 1 atm-ish */
    "auto",
    NULL,
    n_out
);
```

Interpretation of the result:

- `n_out[i]` is the equilibrium mole amount of species `names[i]`
- total metallic iron is

$$
  n_{\mathrm{Fe,metal}} = n_{\mathrm{Fe_bcc}} + n_{\mathrm{Fe_fcc}}
$$

- total wustite phase is

$$
  n_{\mathrm{wus}} = n_{\mathrm{Wus\_FeO}} + n_{\mathrm{Wus\_FeO1p5}}
$$

- wustite composition is

$$
  x = \frac{n_{\mathrm{Wus\_FeO1p5}}}
           {n_{\mathrm{Wus\_FeO}} + n_{\mathrm{Wus\_FeO1p5}}}
$$

- spinel is present if the spinel member amounts are nonzero

For systems containing solution phases like wustite or spinel,
`"auto"` is the right first choice. The code will avoid the old
reduced/nullspace-only path when that is not valid.

### What to extract from an Fe-O-H solve

For end users, the most useful outputs are usually:

- stable condensed assemblage
- gas composition
- total metallic iron
- wustite composition `x`
- whether the system has reduced to:
  - hematite
  - spinel
  - wustite
  - iron

In practice, for reduction studies, the phase-presence pattern and
the ratios

$$
\frac{n_{\mathrm{H_2O}}}{n_{\mathrm{H_2}}}
\qquad \text{or} \qquad
\frac{p_{\mathrm{H_2O}}}{p_{\mathrm{H_2}}}
$$

are often the most informative quantities.

## Where This Fits

This `fprops` `eqm` code is a chemical-reaction equilibrium solver.
Its core problem is: minimize mixture Gibbs energy over species amounts `n`, subject to element balances `A n = b`.
That is exactly the right structure for reactive systems (for example CO/CO2/H2/H2O/O2, or future solid-gas redox systems).

By contrast, `models/thermodynamics.a4l` is mainly a phase-equilibrium/flash framework.
It enforces phase conditions (equal `T`, equal `P`, and optionally equality of component partial Gibbs energies across phases), plus phase-fraction complementarity.
As written, it does not introduce reaction stoichiometry, reaction extents, or elemental conservation constraints that permit chemical conversion between species.

So yes: in its current form, `thermodynamics.a4l` handles phase redistribution of a fixed species set, not chemical reaction equilibrium.
That ASCEND model remains valuable for process-level phase-equilibrium structure and model composition, while `fprops/eqm` is the appropriate core for reaction equilibrium.

### Reactive flash and metallurgical multiphase work

The next planned `reactive_flash` layer should be understood as a
general multiphase reacting-equilibrium framework, not merely a
vapor-liquid flash with reaction.

That distinction matters because there are two quite different
application classes:

- `UNIFAC`-style gas-liquid systems:
  - vapor phase
  - nonideal liquid phase
  - classic `TPz` flash structure
- metallurgical systems:
  - reacting gas
  - metallic liquid or metallic solution phase
  - slag liquid phase
  - possible solid carbon / oxide / spinel / wustite phases

The current `UNIFAC` work is useful because it establishes:

- package-owned phase-model selection
- source-data / rundata separation
- native C preparation and caching
- explicit flash/state interfaces

However, it does **not** by itself provide the thermodynamic models
needed for smelters, BOFs, EAFs, or slag-metal-gas equilibria.

For that reason, the practical metallurgical path remains:

1. current Tier 3 `Fe-O-H`
2. next Tier 4 `Fe-O-H-C`
3. only then broader slag systems such as
   - `Fe-O-H-C-SiO2`
   - `Fe-O-H-C-Al2O3-SiO2`

So the role of `reactive_flash` is:

- useful and directly applicable for gas-liquid reacting systems now
- architecturally useful for metallurgy, provided it is built as a
  general multiphase package/state framework
- not yet sufficient for slag/metal equilibrium until suitable phase
  models exist for those condensed phases

In other words, if `reactive_flash` is built around a generic package
of phase models, it is the right road. If it is built too narrowly as
only a `UNIFAC` vapor-liquid reactor, it will not generalize well to
the Tier 4 and slag work.

## Part A. Common Principles

### 1. Problem statement

Let:

- $n_s$ = number of chemical species.
- $n_e$ = number of conserved elements.
- species index $i \in \{1,\dots,n_s\}$.
- element index $e \in \{1,\dots,n_e\}$.

For species moles
$\mathbf{n} = [n_1,\dots,n_{n_s}]^T \in \mathbb{R}^{n_s}$,
we minimize total Gibbs energy at fixed $T$ and $P$:

$$
\min_{\mathbf{n}}\; G(\mathbf{n})
$$

subject to element balances

$$
\mathbf{A}\,\mathbf{n} = \mathbf{b}
$$

and non-negativity

$$
\mathbf{n} \ge 0.
$$

Here:

- $\mathbf{A}\in\mathbb{R}^{n_e\times n_s}$ with entries $A_{e,i}$.
- $\mathbf{b}\in\mathbb{R}^{n_e}$ with entries $b_e$.
- $A_{e,i}$ = number of atoms of element $e$ in species $i$.
- $b_e$ = total atoms of element $e$ in the system.

So each element balance is

$$
\sum_{i=1}^{n_s} A_{e,i}\,n_i = b_e, \qquad e=1,\dots,n_e.
$$

### 2. Ideal-gas thermodynamics used

For each species $i$:

$$
\mu_i(T,P,\mathbf{n}) = \mu_i^\circ(T,P^\circ) + RT\ln\left(\frac{y_i P}{P^\circ}\right)
$$

with

$$
y_i = \frac{n_i}{n_{\mathrm{tot}}}, \qquad n_{\mathrm{tot}} = \sum_i n_i.
$$

Total Gibbs energy is

$$
G = \sum_i n_i\mu_i.
$$

The standard chemical potentials $\mu_i^\circ$ are computed from the ideal-fluid data in FPROPS.
For ideal gases, the activity is $a_i = y_i P/P^\circ$, so the logarithm argument is dimensionless.
Define the chemical-potential vector
$\boldsymbol{\mu}=[\mu_1,\dots,\mu_{n_s}]^T$.

#### 2.1 How `mu0(T)` is obtained from FPROPS data

In code, `eqm_compute_mu0` calls `eqm_mu0_ideal_source` for each species.
That routine builds an ideal-fluid object and evaluates Gibbs energy at `(T, P0)` as follows:

- compute ideal-gas density from `rho = P0/(R T)`,
- evaluate mass-specific Gibbs energy `g(T, rho)` from the ideal-fluid model,
- convert to molar standard chemical potential with `mu0 = g * M` (where `M` is in kg/mol).

For equilibrium work, it first tries to apply the species `ref0` reference state (`FPROPS_REF_REF0`) so that the absolute Gibbs level is formation-based.
For many species (notably the RPP set), `ref0` is stored as `FPROPS_REF_TPHG` with `(T_ref, p_ref, h_f, g_f)`.

How the formation data enters:

- in `components.a4l`, `Hf` and `Gf` are stored as formation enthalpy/free energy (units `J/g_mole`),
- `convcomp.py` converts these to per-mass values (`h_f0 = Hf/mw`, `g_f0 = Gf/mw`) in generated fluid C data (for example `_rpp.c`),
- during `ideal_prepare(..., FPROPS_REF_REF0)`, FPROPS converts to SI mass basis and sets the ideal reference constants so that ideal `h` and `g` at `(T_ref, p_ref)` match those targets (equivalently it uses `s_f = (h_f-g_f)/T_ref`).

So the temperature dependence comes from the ideal `cp0` model, while the absolute level comes from the `ref0` formation reference.
If a species has missing `ref0` formation terms, `eqm_mu0_ideal_source` falls back to other ideal-source paths, but those may not be formation-consistent across species.

### 3. KKT conditions (equilibrium conditions)

Quick summary: KKT conditions are the first-order optimality conditions for constrained optimization.
They combine four ideas:

- stationarity (zero first derivative of the Lagrangian),
- primal feasibility (original constraints hold),
- dual feasibility (inequality multipliers are nonnegative),
- complementarity (inactive inequality multipliers vanish).

For this equilibrium problem, KKT is the mathematical statement that no infinitesimal, element-balanced, admissible composition change can reduce Gibbs energy further.

Define the Lagrangian with equality multipliers $\boldsymbol\lambda\in\mathbb{R}^{n_e}$ and bound multipliers $\mathbf{s}\in\mathbb{R}^{n_s}_{\ge 0}$:

$$
\mathcal{L}(\mathbf{n},\boldsymbol\lambda,\mathbf{s}) = G(\mathbf{n}) + \boldsymbol\lambda^T(\mathbf{A}\mathbf{n}-\mathbf{b}) - \mathbf{s}^T\mathbf{n}.
$$

At optimum:

$$
\nabla_{\mathbf{n}}\mathcal{L} = \boldsymbol\mu + \mathbf{A}^T\boldsymbol\lambda - \mathbf{s} = 0,
$$
$$
\mathbf{A}\mathbf{n} = \mathbf{b}, \quad \mathbf{n}\ge \mathbf{0}, \quad \mathbf{s}\ge \mathbf{0},
$$
$$
n_i s_i = 0, \qquad i=1,\dots,n_s.
$$

#### 3.1 Why KKT gives equilibrium here

This is a constrained minimization problem:

- objective: minimize $G(\mathbf{n})$,
- linear equalities: $\mathbf{A}\mathbf{n}=\mathbf{b}$,
- bound inequalities: $\mathbf{n}\ge 0$.

KKT conditions are the first-order optimality conditions for exactly this problem class.
For ideal-gas mixtures at fixed $(T,P)$, $G(\mathbf{n})$ is convex on the positive interior $\mathbf{n}>0$, and constraints are linear, so KKT is sufficient for global optimality there.
Near bounds ($n_i\to 0$), the logarithmic terms become singular, so boundary solutions are interpreted via complementarity limits and active-set KKT checks.

Interpretation:

If $n_i>0$ (free species), then complementarity gives $s_i=0$, so $\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i=0$.

If $n_i=0$ (active bound), then $s_i\ge 0$, so the KKT residual is nonnegative: $\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i \ge 0$.

#### 3.2 Physical interpretation of multipliers

$\lambda_e$ acts like an elemental potential for conserved element $e$.
Define the constrained driving force $\tilde{\mu}_i = \mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i$.
At equilibrium, free species satisfy $\tilde{\mu}_i=0$ and bound species satisfy $\tilde{\mu}_i\ge 0$.

So no feasible composition change can reduce $G$ further.

### 4. Why nullspace/reduced coordinates

Quick summary: the nullspace of $\mathbf{A}$ is $\mathcal{N}(\mathbf{A})=\{\mathbf{v}\in\mathbb{R}^{n_s}\mid \mathbf{A}\mathbf{v}=0\}$.
Any step $\Delta\mathbf{n}$ in this nullspace preserves element totals, because $\mathbf{A}(\mathbf{n}+\Delta\mathbf{n})=\mathbf{A}\mathbf{n}$.

Direct optimization in $\mathbf{n}$ has linear constraints $\mathbf{A}\mathbf{n}=\mathbf{b}$.

We build:

- a particular solution $\mathbf{n}_0$ such that $\mathbf{A}\mathbf{n}_0=\mathbf{b}$,
- a nullspace basis $\mathbf{N}$ such that $\mathbf{A}\mathbf{N}=0$.

Then every feasible point is

$$
\mathbf{n} = \mathbf{n}_0 + \mathbf{N}\mathbf{z},
$$

where $\mathbf{z}\in\mathbb{R}^r$ and $r=n_s-\mathrm{rank}(\mathbf{A})$.

Now equality constraints are automatically satisfied; only positivity remains.

#### 4.1 Geometric picture

The feasible set from element balances is an affine subspace:

$$
\mathcal{F}=\{\mathbf{n}\in\mathbb{R}^{n_s}\mid \mathbf{A}\mathbf{n}=\mathbf{b}\}.
$$

Here, "affine subspace" means a shifted linear subspace: it can be written as $\mathcal{F}=\mathbf{n}_0+\mathcal{N}(\mathbf{A})$.
So it is flat (like a plane/line in higher dimensions), but not required to pass through the origin.

- $\mathbf{n}_0$ is one point on this subspace.
- Columns of $\mathbf{N}$ span directions tangent to $\mathcal{F}$.
- Moving along $\mathbf{N}\mathbf{z}$ keeps all element totals unchanged.

So the solver is doing unconstrained optimization on the feasible manifold, then clipping step size to respect non-negativity.

#### 4.2 Connection to reaction extents

Each nullspace direction $\boldsymbol\nu$ satisfies $\mathbf{A}\boldsymbol\nu=0$, which is exactly the condition for a balanced stoichiometric reaction vector.
So reduced coordinates are generalized reaction extents.

Important: the reduced coordinates are not unique. If $\mathbf{z}$ is replaced by $\hat{\mathbf{z}}=\mathbf{Q}^{-1}\mathbf{z}$ for any invertible matrix $\mathbf{Q}$, with basis $\hat{\mathbf{N}}=\mathbf{N}\mathbf{Q}$, the same feasible compositions are represented.
So the variables are "reaction-extent-like" coordinates on the same feasible manifold, and different bases are just different coordinate systems.

If $r=1$, this becomes the familiar single-extent form:

$$
\mathbf{n}=\mathbf{n}_0+\boldsymbol\nu\,\xi.
$$

Programmatically, independent $\boldsymbol\nu$ vectors are extracted from $\mathbf{A}$ by row-reduction, then assembled into $\mathbf{N}$.

#### 4.3 How reduced space follows from species formulae

Start from:

- a species list (for example: CO, CO2, O2),
- chemical formulae for each species,
- a chosen element set (here C and O).

From formula parsing, build the element matrix with rows = elements and columns = species:

`A = [[1, 1, 0], [1, 2, 2]]`.

where column order is $(\mathrm{CO},\mathrm{CO}_2,\mathrm{O}_2)$.
For example:

- CO contributes C:1, O:1,
- CO2 contributes C:1, O:2,
- O2 contributes C:0, O:2.

If feed totals are $b_{\mathrm{C}}, b_{\mathrm{O}}$, feasibility is $\mathbf{A}\mathbf{n}=\mathbf{b}$ with $\mathbf{b}=[b_{\mathrm{C}},\,b_{\mathrm{O}}]^T$.

The nullspace dimension is $r=n_s-\mathrm{rank}(\mathbf{A})=3-2=1$.

One null vector is $\boldsymbol\nu=[2,\,-2,\,1]^T$, and it satisfies $\mathbf{A}\boldsymbol\nu=\mathbf{0}$.

So every feasible composition is $\mathbf{n}=\mathbf{n}_0+\boldsymbol\nu\,\xi$.

This is exactly the extent form for the balanced reaction

$2\,\mathrm{CO}+\mathrm{O}_2\rightleftharpoons 2\,\mathrm{CO}_2$.

General case is the same workflow:

1. Parse formulae $\rightarrow$ fill $\mathbf{A}$.
2. Solve $\mathbf{A}\mathbf{n}_0=\mathbf{b}$ for one feasible $\mathbf{n}_0$.
3. Compute nullspace basis $\mathbf{N}$ of $\mathbf{A}$.
4. Optimize only in $\mathbf{z}$ with $\mathbf{n}=\mathbf{n}_0+\mathbf{N}\mathbf{z}$.

So reduced-space variables are not an arbitrary numerical trick; they are the algebraic expression of all element-balanced composition changes implied by the formulae.

### 5. Reduced objective, gradient, Hessian

Define

$$
\phi(\mathbf{z}) = G(\mathbf{n}_0 + \mathbf{N}\mathbf{z}).
$$

Gradient:

$$
\nabla_{\mathbf{z}}\phi = \mathbf{N}^T\boldsymbol\mu.
$$

So stationarity is exactly

$$
\mathbf{N}^T\boldsymbol\mu = \mathbf{0}.
$$

Interpretation: $\mathbf{N}^T\boldsymbol\mu$ is the Gibbs gradient projected onto feasible directions.
At equilibrium, this projected gradient must vanish, meaning there is no downhill direction left that also preserves element balances.

Hessian used in code:

$$
\mathbf{H} = RT\left(\mathbf{N}^T\mathrm{diag}(1/n_i)\mathbf{N} - \frac{\mathbf{c}\mathbf{c}^T}{n_{\mathrm{tot}}}\right),
$$

with

$$
\mathbf{c} = \mathbf{N}^T\mathbf{1}.
$$

This comes from differentiating the ideal-mixture $\mu_i$ with respect to composition.
In numerical terms, $\mathbf{H}$ is the local curvature of $G$ restricted to feasible composition changes.
That is why Newton steps in $\mathbf{z}$ are usually fast when $\mathbf{H}$ is well-conditioned.

## Part B. Solver Pathways

### 6. Main solve modes

From `eqm_solve_elements`/`eqm_solve`, key algorithms are:

- `ipopt*` (NLP in full coordinates)
- `slsqp` (NLP fallback)
- `reduced` (nullspace reduced-space Newton)
- `auto_reduced` (try reduced first, then fallback path)

Pathway grouping:

- Reduced-space pathway (primary): `reduced` (with optional 1D special solve, continuation, and active-set boundary handling).
- Full-space pathway (secondary): `ipopt*` with `slsqp` fallback.

For low-temperature boundary-heavy cases, `reduced` is now the primary robust path.

### 7. Reduced Newton method (interior part)

Given current $\mathbf{z}$:

1. Build $\mathbf{n}=\mathbf{n}_0+\mathbf{N}\mathbf{z}$.
2. Evaluate $\phi,\nabla\phi,\mathbf{H}$.
3. Solve damped Newton system

$$
(\mathbf{H}+\lambda\mathbf{I})\Delta\mathbf{z} = -\nabla\phi.
$$

Here $\lambda\ge 0$ is a damping term used when curvature is weak or near-singular.
It shifts the step from aggressive Newton behavior toward safer gradient-like behavior.

4. Enforce positivity via max step $\alpha_{\max}$ from

$$
n_i + \alpha\Delta n_i > n_{\mathrm{floor}}.
$$

with

$$
\Delta\mathbf{n}=\mathbf{N}\,\Delta\mathbf{z}.
$$

5. Backtracking line search for descent in $\phi$.

These two safeguards (damping + line search) are the globalization layer: they improve convergence from imperfect initial guesses, not just from near-solution starts.

### 8. 1D special case ($r=1$)

If only one reduced degree of freedom exists, the nullspace basis has one column; call it $\mathbf{v}$.
Then write

$$
\mathbf{n}(z)=\mathbf{n}_0+\mathbf{v}z.
$$

Then equilibrium is root of

$$
\Phi(z)=\mathbf{v}^T\boldsymbol\mu(\mathbf{n}(z))=0.
$$

Code uses robust bracketing/bisection (with long-double support and edge handling) for this case.

### 9. Continuation/homotopy used

This is a robustness layer for the reduced-space pathway.
It is not part of the reduced-space definition itself; it is a practical globalization strategy when direct solve at target $T$ is difficult.

To improve robustness at low temperature:

- Temperature continuation uses relative ladder:

$$
T_{k+1}=0.82\,T_k, \qquad T_0=2.5\,T_{\mathrm{target}},
$$

until reaching $T_{\mathrm{target}}$.

- Positivity floor continuation uses decreasing floors (e.g. $10^{-12}\to10^{-120}$).

This avoids starting the hardest solve cold.

### 10. Active-set seed (boundary-aware)

When interior reduced solve fails near boundary, code now runs an active-set seed routine:

- Split species into active set $\mathcal{A}$ (pinned at $n_i=n_{\mathrm{floor}}$) and free set $\mathcal{F}$.
- Solve reduced problem on free species only.
- Compute reduced gradients as $r_i = (\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i)/(RT)$.
- Recover `lambda` from free species set `F` by solving `A_F^T lambda ≈ -mu_F`, exactly if dimensions permit, otherwise as a least-squares system.

- Pivot rules:
  - add species to active set if free species is near bound and $r_i>0$,
  - drop species from active set if $r_i<0$.

Then polish with full reduced solve.

This is the main reason low-temperature mixed cases (like CO/CO2/H2O/H2/O2) now converge down to 298 K.

### 10.1 NOx study lessons for reduced-space startup

The humid-air NOx gas basis exposed an important extra requirement for
the reduced-space path: startup must be invariant, or nearly invariant,
to species ordering.

What was observed during the NOx study:

- the same physical humid-air NOx problem could succeed or fail in
  `auto_reduced` depending on species order alone
- this was not a thermodynamic multiplicity issue in the direct solve;
  it came from the reduced-space construction and boundary startup logic
- once a first reduced solution was accepted, continuation could still
  be driven onto different low-temperature branches if the startup basis
  and active-set seed were poor

The fixes that proved important were:

- choose reduced pivots/basis columns using a target-weighted ordering,
  not raw species index order
- compute the particular feasible point `n0` from the chosen pivots, so
  the reduced map and the pivot structure stay consistent
- classify active/free species using the same near-boundary
  complementarity logic used by solution validation, rather than only a
  small-amount cutoff
- when `n_init == NULL`, build a deterministic bootstrap composition and
  feed that into reduced startup instead of treating null-init as "no
  seed at all"
- score candidate active-set pivots by post-solve KKT quality/objective,
  rather than taking the first legal add/drop move

Current FPROPS status for this NOx basis:

- direct package-backed solves for the demo gas basis now succeed to
  `300 K`
- warm starts from physically reasonable higher-temperature states do
  not trap the direct reduced solver on the wrong low-temperature branch
- the remaining low-temperature discrepancy is no longer in the direct
  FPROPS equilibrium kernel itself; it is in the ASCEND embedding and
  outer-solver treatment of the returned trace-species rows

### 11. Boundary-KKT validation

If strict interior stationarity check fails, we also accept solutions satisfying bound-KKT logic:

- free species: $|r_i| \le \varepsilon_{\mathrm{free}}$,
- active species: $r_i \ge -\varepsilon_{\mathrm{dual}}$.

Current tolerances in code are explicitly defined constants:

$$
\varepsilon_{\mathrm{free}} = \varepsilon_{\mathrm{dual}} = 2\times10^{-2}.
$$

These are numerical acceptance tolerances for the nonlinear solver, not physical constants.

Active-species detection also uses

$$
n_i \le \max\left(10^{-60},\,\eta\,n_{\mathrm{tot}}\right), \qquad \eta=10^{-22}.
$$

For the humid-air NOx work, this validation logic was also used as the
basis for active/free classification in startup. That is a better guide
than a raw concentration cutoff, because it is tied to complementarity:
small positive species can be treated as effectively active only when
their reduced-gradient sign is also consistent with a boundary KKT
state.

For the secondary full-space interior-point pathway, see Appendix A.

## Part C. Validation and Operations

### 12. What “correctness” means here

For ideal-gas equilibrium we check:

1. Element residuals: $\|\mathbf{A}\mathbf{n} - \mathbf{b}\|$ small.
2. KKT stationarity (interior or bound-aware as above).
3. Cross-check against external reference (Cantera), reaction by reaction for a chosen independent reaction basis $\boldsymbol\nu_j$ (for example, columns of $\mathbf{N}$):

$$
\Delta\log_{10}K_j = \log_{10}K_{j,\mathrm{FPROPS}} - \log_{10}K_{j,\mathrm{ref}},
\qquad
j=1,\dots,r.
$$

Here each reaction constant satisfies $\ln K_j = -\Delta_r G_j^\circ/(RT)$, with $\Delta_r G_j^\circ = \sum_i \nu_{i,j}\mu_i^\circ$.

Small $\Delta\log_{10}K_j$ and small composition differences indicate good agreement.

### 12.1 Reaktoro-clone validation path (`Ni`/`NiO`/`H2`/`O2`/`H2O`)

To separate implementation errors from source-data differences, `fprops` also includes a dedicated shomate source:

- `reaktoro_clone_supcrt98`

This source is fitted against Reaktoro/SUPCRT98 standard-state Gibbs data for:

- `Ni`, `NiO`, `hydrogen`, `oxygen`, `water`.

The intent is not to define a new recommended thermodynamic database.
The intent is implementation validation: if FPROPS and Reaktoro use nearly the same $\mu_i^\circ(T)$ inputs, equilibrium outputs should match closely.

Current check method:

1. Compare species $\mu_i^\circ(T)$ directly between providers.
2. Fit elemental-potential shifts and inspect non-elemental residuals.
3. Inspect reaction-level mismatch
   $\Delta\Delta G^\circ = \sum_i \nu_i\left(\mu_{i,\mathrm{FPROPS}}^\circ - \mu_{i,\mathrm{Reaktoro}}^\circ\right)$
   for balanced reactions.

How to run:

```bash
python3 models/johnpye/fprops/test/eqm_mu0_reconcile.py \
  --a 'fprops:reaktoro_clone_supcrt98' \
  --b 'reaktoro:supcrt98' \
  --preset nio_h2o \
  --temps-c 25,100,200,300,400,500,600,700,800,900,1000,1100,1200 \
  --reaktoro-shell-prefix 'eval "$(micromamba shell hook --shell bash)" && micromamba activate'
```

Interpretation guide:

- If reaction-level mismatch is small (order `1-50 J/mol` across this range), then remaining solver differences are usually negligible for engineering equilibrium predictions.
- Larger discrepancies then mostly come from database/model differences (not optimizer failure).

Notes:

- Auto source routing in `eqm` now prefers this shomate clone data when `source=reaktoro_clone_supcrt98` is requested, so clone gases are not silently replaced by unrelated default ideal-gas data.
- For production calculations, use your chosen physical database (for example OECD/NIST-consistent sets); keep `reaktoro_clone_supcrt98` as a verification harness.

### 13. Practical debug hook

Set

```bash
FPROPS_EQM_ACTIVESET_TRACE=1
```

to print active-set iterations, pivots, and residuals from the low-temperature fallback path.

### 14. Scope note

This document describes current ideal-gas chemical equilibrium implementation.
Solid-gas equilibria (e.g. iron oxide reduction steps) can be built on the same constrained minimization framework, but add phase-activity models for solids.

## Part D. Equation-to-Code Map

### 15. Equation-to-Code Map

This section maps the key equations in this note to the main implementation points in `eqm.c`.

#### 15.1 Thermodynamic state equations

Equation: $\mu_i = \mu_i^\circ + RT\ln\left(\frac{y_i P}{P^\circ}\right)$.
Code:
- `eqm_compute_mu0` and `eqm_mu0_ideal_source` compute $\mu_i^\circ(T, P^\circ)$.
- `eqm_reduced_eval_obj_mu` computes $y_i$, $\mu_i$, and $G = \sum_i n_i\mu_i$.

#### 15.2 Feasible-set parameterization

Equation: $\mathbf{n} = \mathbf{n}_0 + \mathbf{N}\mathbf{z} $, with $ \mathbf{A}\mathbf{n}_0 = \mathbf{b} $ and $ \mathbf{A}\mathbf{N} = 0$ .
Code:
- `eqm_solve_particular` computes $\mathbf{n}_0$ .
- `eqm_rref` + `eqm_fill_nullspace` build $\mathbf{N}$ .
- `eqm_reduced_compute_n` evaluates $\mathbf{n}(\mathbf{z})$ .

#### 15.3 Reduced gradient and Hessian

Equations: $\nabla_{\mathbf{z}}\phi = \mathbf{N}^T\boldsymbol\mu$ , and $\mathbf{H} = RT\left(\mathbf{N}^T\mathrm{diag}(1/n_i)\mathbf{N} - \frac{\mathbf{c}\mathbf{c}^T}{n_{\mathrm{tot}}}\right)$ with $\mathbf{c} = \mathbf{N}^T\mathbf{1}$ .
Code:
- `eqm_reduced_eval_grad_hess`.

#### 15.4 Newton step and globalization

Equations: $(\mathbf{H} + \lambda\mathbf{I})\Delta\mathbf{z} = -\nabla\phi$ , plus positivity-limited line search constraint $\mathbf{n} + \alpha\Delta\mathbf{n} > n_{\mathrm{floor}}$ .
Code:
- Newton loop in `eqm_reduced_solve_source_init_once`.
- linear solve via `eqm_dense_solve`.
- positivity handling and backtracking in the same routine.

#### 15.5 1D nullspace special case

Equation: $\Phi(z) = \mathbf{v}^T\boldsymbol\mu(\mathbf{n}_0 + \mathbf{v}z) = 0$ .
Code:
- `eqm_reduced_solve_r1`.
- helper evaluators: `eqm_reduced_eval_phi_r1`, `eqm_reduced_eval_phi_edge`, `eqm_reduced_try_edge_root`.

#### 15.6 Continuation/homotopy

Equations: $T_{k+1} = 0.82\,T_k$ with $T_0 = 2.5\,T_{\mathrm{target}}$, and floor schedule $n_{\mathrm{floor}}:10^{-12}\to10^{-120}$ .
Code:
- `eqm_reduced_solve_source_init`.

#### 15.7 Active-set reduced KKT quantities

Equation: $r_i = \frac{\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i}{RT}$ , where $\boldsymbol\lambda$ is computed from free-species equations.
Code:
- `eqm_reduced_eval_reduced_gradients`.
- active-set pivot loop in `eqm_reduced_active_set_seed`.

#### 15.8 Boundary-KKT acceptance

Conditions: $|r_i| \le \varepsilon_{\mathrm{free}}$ for $i \in \mathcal{F}$ and $r_i \ge -\varepsilon_{\mathrm{dual}}$ for $i \in \mathcal{A} $, with active cutoff $ n_i \le \max\left(10^{-60},\,\eta\,n_{\mathrm{tot}}\right)$ and $\eta = 10^{-22}$ .
Code:
- `eqm_validate_solution_bounds`.
Constants used in code: `EQM_BOUND_KKT_FREE_TOL`, `EQM_BOUND_KKT_DUAL_TOL`, `EQM_BOUND_ACTIVE_CUTOFF_FRAC`.

#### 15.9 Final solver dispatch

Logic:
- try reduced path (`reduced` or `auto_reduced`),
- validate by interior or bound-KKT tests,
- in `auto_reduced`, fall back to generic `auto` pipeline if needed.
Code:
- `eqm_solve` and `eqm_solve_elements`.

## Appendix A. Full-space interior-point path (secondary)

The full-space IPOPT path keeps all `n_i` strictly positive and solves a barrier sequence:
`min G(n) - tau * sum_i ln(n_i)` subject to `A n = b`, with `tau > 0` reduced toward zero.

Equivalent perturbed KKT form is:
`mu + A^T lambda - s = 0`, `A n - b = 0`, and `n_i s_i = tau` with `n_i > 0`, `s_i > 0`.
As `tau -> 0`, this tends to the original complementarity relation `n_i s_i = 0`.

In current FPROPS equilibrium work, this full-space pathway is useful as fallback/cross-check, but reduced-space plus active-set handling has been more robust for low-temperature, boundary-dominated chemistry.

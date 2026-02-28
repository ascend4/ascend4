# FPROPS Chemical Equilibrium: Thermodynamics and Algorithms

This note explains the current ideal-gas equilibrium implementation in `fprops` at a second-year thermo level, while also showing the numerical methods used in code.

## 1. Problem statement

For species moles
$\mathbf{n} = [n_1,\dots,n_{n_s}]^T$,
we minimize total Gibbs energy at fixed $T,P$:

$$
\min_{\mathbf{n}}\; G(\mathbf{n})
$$

subject to element balances:

$$
\mathbf{A}\,\mathbf{n} = \mathbf{b}
$$

and non-negativity:

$$
\mathbf{n} \ge 0.
$$

Here:

- $A_{e i}$ = number of atoms of element $e$ in species $i$.
- $b_e$ = total atoms of element $e$ in the system.

## 2. Ideal-gas thermodynamics used

For each species $i$:

$$
\mu_i(T,P,\mathbf{n}) = \mu_i^\circ(T,P^\circ) + RT\ln\!\left(\frac{y_i P}{P^\circ}\right)
$$

with

$$
y_i = \frac{n_i}{n_{\text{tot}}}, \qquad n_{\text{tot}} = \sum_i n_i.
$$

Total Gibbs energy is

$$
G = \sum_i n_i\mu_i.
$$

The standard chemical potentials $\mu_i^\circ$ are computed from the ideal-fluid data in FPROPS.

## 3. KKT conditions (equilibrium conditions)

Define Lagrangian with equality multipliers $\boldsymbol\lambda$ and bound multipliers $\mathbf{s}\ge 0$:

$$
\mathcal{L}(\mathbf{n},\boldsymbol\lambda,\mathbf{s}) = G(\mathbf{n}) + \boldsymbol\lambda^T(\mathbf{A}\mathbf{n}-\mathbf{b}) - \mathbf{s}^T\mathbf{n}.
$$

At optimum:

$$
\nabla_{\mathbf{n}}\mathcal{L} = \boldsymbol\mu + \mathbf{A}^T\boldsymbol\lambda - \mathbf{s} = 0,
$$
$$
\mathbf{A}\mathbf{n} = \mathbf{b}, \quad \mathbf{n}\ge 0, \quad \mathbf{s}\ge 0,
$$
$$
 n_i s_i = 0\;\;\forall i.
$$

Interpretation:

- If $n_i>0$ (free species), then $s_i=0$, so $\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i=0$.
- If $n_i=0$ (active bound), then $s_i\ge 0$, so reduced gradient is nonnegative.

## 4. Why nullspace/reduced coordinates

Direct optimization in $\mathbf{n}$ has linear constraints $\mathbf{A}\mathbf{n}=\mathbf{b}$.

We build:

- a particular solution $\mathbf{n}_0$ such that $\mathbf{A}\mathbf{n}_0=\mathbf{b}$,
- a nullspace basis $\mathbf{N}$ such that $\mathbf{A}\mathbf{N}=0$.

Then every feasible point is

$$
\mathbf{n} = \mathbf{n}_0 + \mathbf{N}\mathbf{z},
$$

where $\mathbf{z}\in\mathbb{R}^r$ and $r=n_s-\text{rank}(\mathbf{A})$.

Now equality constraints are automatically satisfied; only positivity remains.

## 5. Reduced objective, gradient, Hessian

Define

$$
\phi(\mathbf{z}) = G(\mathbf{n}_0 + \mathbf{N}\mathbf{z}).
$$

Gradient:

$$
\nabla\phi = \mathbf{N}^T\boldsymbol\mu.
$$

So stationarity is exactly

$$
\mathbf{N}^T\boldsymbol\mu = 0.
$$

Hessian used in code:

$$
\mathbf{H} = RT\left(\mathbf{N}^T\operatorname{diag}(1/n_i)\mathbf{N} - \frac{\mathbf{c}\mathbf{c}^T}{n_{\text{tot}}}\right),
$$

with

$$
\mathbf{c} = \mathbf{N}^T\mathbf{1}.
$$

This comes from differentiating the ideal-mixture $\mu_i$ w.r.t. composition.

## 6. Main solve modes

From `eqm_solve_elements`/`eqm_solve`, key algorithms are:

- `ipopt*` (NLP in full coordinates)
- `slsqp` (NLP fallback)
- `reduced` (nullspace reduced-space Newton)
- `auto_reduced` (try reduced first, then fallback path)

For low-$T$ boundary-heavy cases, `reduced` is now the primary robust path.

## 7. Reduced Newton method (interior part)

Given current $\mathbf{z}$:

1. Build $\mathbf{n}=\mathbf{n}_0+\mathbf{N}\mathbf{z}$.
2. Evaluate $\phi,\nabla\phi,\mathbf{H}$.
3. Solve damped Newton system

$$
(\mathbf{H}+\lambda\mathbf{I})\Delta\mathbf{z} = -\nabla\phi.
$$

4. Enforce positivity via max step $\alpha_{\max}$ from

$$
n_i + \alpha\Delta n_i > n_{\text{floor}}.
$$

5. Backtracking line search for descent in $\phi$.

## 8. 1D special case ($r=1$)

If only one reduced degree of freedom exists, write

$$
\mathbf{n}(z)=\mathbf{n}_0+\mathbf{v}z.
$$

Then equilibrium is root of

$$
\Phi(z)=\mathbf{v}^T\boldsymbol\mu(\mathbf{n}(z))=0.
$$

Code uses robust bracketing/bisection (with long-double support and edge handling) for this case.

## 9. Continuation/homotopy used

To improve robustness at low $T$:

- Temperature continuation uses relative ladder:

$$
T_{k+1}=0.82\,T_k, \qquad T_0=2.5\,T_{\text{target}},
$$

until reaching $T_{\text{target}}$.

- Positivity floor continuation uses decreasing floors (e.g. $10^{-12}\to10^{-120}$).

This avoids starting the hardest solve cold.

## 10. Active-set seed (boundary-aware)

When interior reduced solve fails near boundary, code now runs an active-set seed routine:

- Split species into active set $\mathcal{A}$ (pinned at $n_i=n_{\text{floor}}$) and free set $\mathcal{F}$.
- Solve reduced problem on free species only.
- Compute reduced gradients

$$
r_i = \frac{\mu_i + (\mathbf{A}^T\boldsymbol\lambda)_i}{RT}.
$$

- Pivot rules:
  - add species to active set if free species is near bound and $r_i>0$,
  - drop species from active set if $r_i<0$.

Then polish with full reduced solve.

This is the main reason low-$T$ mixed cases (like CO/CO2/H2O/H2/O2) now converge down to 298 K.

## 11. Boundary-KKT validation

If strict interior stationarity check fails, we also accept solutions satisfying bound-KKT logic:

- free species: $|r_i| \le \varepsilon_{\text{free}}$,
- active species: $r_i \ge -\varepsilon_{\text{dual}}$.

Current tolerances in code are explicitly defined constants:

$$
\varepsilon_{\text{free}} = \varepsilon_{\text{dual}} = 2\times10^{-2}.
$$

## 12. What “correctness” means here

For ideal-gas equilibrium we check:

1. Element residuals: $\|\mathbf{A}\mathbf{n}-\mathbf{b}\|$ small.
2. KKT stationarity (interior or bound-aware as above).
3. Cross-check against external reference (Cantera):

$$
\Delta\log_{10}K = \log_{10}K_{\text{FPROPS}} - \log_{10}K_{\text{ref}}.
$$

Small $\Delta\log_{10}K$ and small composition differences indicate good agreement.

## 13. Practical debug hook

Set

```bash
FPROPS_EQM_ACTIVESET_TRACE=1
```

to print active-set iterations, pivots, and residuals from the low-$T$ fallback path.

## 14. Scope note

This document describes current ideal-gas chemical equilibrium implementation.
Solid-gas equilibria (e.g. iron oxide reduction steps) can be built on the same constrained minimization framework, but add phase-activity models for solids.

## 15. Equation-to-Code Map

This section maps the key equations in this note to the main implementation points in `eqm.c`.

### 15.1 Thermodynamic state equations

- Equation:
  $$
  \mu_i = \mu_i^\circ + RT\ln\!\left(\frac{y_iP}{P^\circ}\right)
  $$
  Code:
  - `eqm_compute_mu0` and `eqm_mu0_ideal_source` compute $\mu_i^\circ(T,P^\circ)$.
  - `eqm_reduced_eval_obj_mu` computes $y_i$, $\mu_i$, and $G=\sum_i n_i\mu_i$.

### 15.2 Feasible-set parameterization

- Equation:
  $$
  \mathbf{n}=\mathbf{n}_0+\mathbf{N}\mathbf{z}, \qquad \mathbf{A}\mathbf{n}_0=\mathbf{b}, \qquad \mathbf{A}\mathbf{N}=0
  $$
  Code:
  - `eqm_solve_particular` computes $\mathbf{n}_0$.
  - `eqm_rref` + `eqm_fill_nullspace` build $\mathbf{N}$.
  - `eqm_reduced_compute_n` evaluates $\mathbf{n}(\mathbf{z})$.

### 15.3 Reduced gradient and Hessian

- Equations:
  $$
  \nabla\phi=\mathbf{N}^T\boldsymbol\mu
  $$
  $$
  \mathbf{H}=RT\!\left(\mathbf{N}^T\operatorname{diag}(1/n_i)\mathbf{N}
  -\frac{\mathbf{c}\mathbf{c}^T}{n_{\text{tot}}}\right), \quad \mathbf{c}=\mathbf{N}^T\mathbf{1}
  $$
  Code:
  - `eqm_reduced_eval_grad_hess`.

### 15.4 Newton step and globalization

- Equations:
  $$
  (\mathbf{H}+\lambda\mathbf{I})\Delta\mathbf{z}=-\nabla\phi
  $$
  plus positivity-limited line search on
  $$
  \mathbf{n}+\alpha\Delta\mathbf{n}>n_{\text{floor}}.
  $$
  Code:
  - Newton loop in `eqm_reduced_solve_source_init_once`.
  - linear solve via `eqm_dense_solve`.
  - positivity handling and backtracking in the same routine.

### 15.5 1D nullspace special case

- Equation:
  $$
  \Phi(z)=\mathbf{v}^T\boldsymbol\mu(\mathbf{n}_0+\mathbf{v}z)=0
  $$
  Code:
  - `eqm_reduced_solve_r1`.
  - helper evaluators: `eqm_reduced_eval_phi_r1`, `eqm_reduced_eval_phi_edge`, `eqm_reduced_try_edge_root`.

### 15.6 Continuation/homotopy

- Equations:
  $$
  T_{k+1}=0.82\,T_k,\qquad T_0=2.5\,T_{\text{target}}
  $$
  and floor schedule $n_{\text{floor}}:10^{-12}\to10^{-120}$.
  Code:
  - `eqm_reduced_solve_source_init`.

### 15.7 Active-set reduced KKT quantities

- Equation:
  $$
  r_i=\frac{\mu_i+(\mathbf{A}^T\boldsymbol\lambda)_i}{RT}
  $$
  where $\boldsymbol\lambda$ is computed from free-species equations.
  Code:
  - `eqm_reduced_eval_reduced_gradients`.
  - active-set pivot loop in `eqm_reduced_active_set_seed`.

### 15.8 Boundary-KKT acceptance

- Conditions:
  $$
  |r_i|\le\varepsilon_{\text{free}}\quad(i\in\mathcal{F}),\qquad
  r_i\ge-\varepsilon_{\text{dual}}\quad(i\in\mathcal{A})
  $$
  with active cutoff
  $$
  n_i \le \max\!\left(10^{-60},\,\eta\,n_{\text{tot}}\right),\quad \eta=10^{-22}.
  $$
  Code:
  - `eqm_validate_solution_bounds`.
  - constants:
    - `EQM_BOUND_KKT_FREE_TOL`
    - `EQM_BOUND_KKT_DUAL_TOL`
    - `EQM_BOUND_ACTIVE_CUTOFF_FRAC`

### 15.9 Final solver dispatch

- Logic:
  - try reduced path (`reduced` or `auto_reduced`),
  - validate by interior or bound-KKT tests,
  - in `auto_reduced`, fall back to generic `auto` pipeline if needed.
  Code:
  - `eqm_solve` and `eqm_solve_elements`.

# FPROPS Fe-O / Fe-O-H Implementation Note

## 1. Purpose

This note records the current implementation status for the `Fe-O`
backbone (Tier 2) and the first `Fe-O-H` extension (Tier 3) in
FPROPS.

The earlier version of this file had grown into a debugging log.
That material has now been condensed into the present-state summary
below:

- what is implemented
- what is actively validated
- what remains approximate
- what is good enough to use going forward

Current position:

- Tier 2 `Fe-O` is implemented and usable
- Tier 3 `Fe-O-H` is implemented and in good enough agreement with
  the corrected Spreitzer/Schenk $\mathrm{H_2}$ traces to proceed
- the main remaining weakness is the $\mathrm{wustite|spinel}$ side of the
  oxide ladder

## 2. Recommended Working Position

Use the current Tier 3 model as the baseline for:

- Tier 4 `Fe-O-C-H`
- further Fe-O-H equilibrium studies
- hydrogen reduction-boundary work centered on
  $\mathrm{Fe|wustite}$

Confidence level by boundary:

- high: gas-side reference-state handling
- high: $\mathrm{Fe|wustite}$
- moderate: $\mathrm{Fe|spinel}$
- moderate-to-lower: $\mathrm{wustite|spinel}$

The main unresolved issue is no longer the gas side. It is the
reduced spinel model.

## 3. Current Thermodynamic Model

### 3.1 Gas species

For Tier 3 work, the preferred gas source is:

- `helmholtz+ref0:`

This now gives chemistry-compatible standard states for:

- `H2`
- `H2O`
- `O2`

The old gas-reference problems were real and have been corrected.
Reaction-level agreement is now good between:

- `helmholtz+ref0:`
- corrected Moran and Shapiro ideal-gas data
- corrected `ideal+ref0:RPP`

For the reaction

$$
\mathrm{H_2} + \tfrac12 \mathrm{O_2} \rightarrow \mathrm{H_2O},
$$

these sources now agree closely on $\Delta G^\circ(T)$.

### 3.2 Metallic iron

Metallic iron is represented by:

- `Fe_bcc`
- `Fe_fcc`

with unary Hillert-Jarl magnetic corrections.

This side is now in good shape:

- the `bcc/fcc` crossover occurs near the expected $912^\circ \mathrm{C}$
- the metallic side is no longer the dominant source of error

### 3.3 Wustite

Wustite is implemented as the accepted Hidayat et al. (2015)
Bragg-Williams binary solution:

$$
(1-x)\,\mathrm{FeO} + x\,\mathrm{FeO}_{1.5}
$$

with

$$
g(T,x) =
(1-x) g^\circ_{\mathrm{FeO}}(T)
+ x g^\circ_{\mathrm{FeO}_{1.5}}(T)
+ RT\left[(1-x)\ln(1-x) + x\ln x\right]
+ g^{ex}(T,x)
$$

and

$$
g^{ex}(T,x) =
X_{\mathrm{FeO}} X_{\mathrm{FeO}_{1.5}}
\left(q^{00} + q^{10} X_{\mathrm{FeO}}\right)
$$

with:

- `q^{00} = -59412.8 J/mol`
- `q^{10} = 42676.8 J/mol`

Important historical point:

- the largest early Tier 2 error came from misreading this excess
  term as a Redlich-Kister form
- once corrected, the $\mathrm{Fe|wustite}$ boundary moved
  into close agreement with Hidayat

### 3.4 Spinel / magnetite side

Spinel is currently represented by a reduced Fe-only spinel CEF
slice derived from Degterov et al. (2001), with the `Fe3O4`
endmember adjusted to the Hidayat 2015 value.

This model includes:

- tetrahedral $\mathrm{Fe^{2+}}$ / $\mathrm{Fe^{3+}}$
- octahedral $\mathrm{Fe^{2+}}$ / $\mathrm{Fe^{3+}}$ / vacancy
- configurational entropy
- a unary magnetic term

But it is still a reduced model, not the full spinel treatment of
the Hidayat/FactSage Fe-O assessment.

This is the weakest remaining part of the present Tier 3 model.

### 3.5 Hematite

`Fe2O3` is still handled on a pragmatic condensed basis with a unary
magnetic correction. It is adequate for the present Tier 3 scope,
but not yet the focus of the remaining mismatch.

## 4. Equilibrium Formulation

The key architectural point from Tier 2 remains:

- condensed solution phases are handled in full-space Gibbs
  minimization
- wustite is solved through endmember amounts rather than an explicit
  top-level `x` variable

This keeps:

- element balances linear
- the overall optimization framework intact

The engine now supports:

- pure condensed phases
- ideal gas species
- binary condensed solutions
- the reduced Fe-only spinel phase

## 5. Active Validation

This section records the validation that should still be treated as
active and relevant.

### 5.1 Tier 2 Fe-O validation: Hidayat

The following are now well supported:

- wustite thermodynamics
- $\mathrm{Fe|wustite}$ boundary
- iron `bcc/fcc` crossover

The important Hidayat-side lesson was:

- the wustite implementation bug is fixed
- the metallic side is no longer the dominant issue
- the remaining Tier 2 discrepancy is localized to the oxide ladder,
  not the gas side

### 5.2 Tier 2 Fe-O validation: O'Neill (1988)

The OCR copy used for this work is:
[oneill-1988-ocr.pdf](/home/john/ascend/models/johnpye/fprops/test/oneill-1988-ocr.pdf)

The useful comparison is on the relative oxygen-potential basis

$$
\mu_{O_2}^{rel} = 2\lambda_O - \mu^\circ_{O_2}(T,p^\circ)
$$

not on raw absolute $2\lambda_O$.

#### $\mathrm{Fe|wustite}$

This boundary now agrees very well with O'Neill:

| T (°C) | Model $\mu_{O_2}^{rel}$ | O'Neill | Delta |
|---:|---:|---:|---:|
| 570 | -418378.0 | -418172.3 | -205.7 |
| 600 | -414566.5 | -414362.5 | -204.0 |
| 700 | -401882.3 | -401643.2 | -239.1 |
| 800 | -388965.7 | -388725.7 | -240.0 |
| 900 | -376005.0 | -375619.9 | -385.1 |
| 1000 | -362870.7 | -362431.9 | -438.9 |

All values are in `J/mol O2`.

This is strong evidence that the current
$\mathrm{Fe|wustite}$ boundary is already correctly placed
on the condensed-side oxygen-potential scale.

#### $\mathrm{wustite|spinel}$

This is where the remaining oxide-side problem sits.

Using the same O'Neill basis, the current model near the invariant
region is still too oxidized by several `kJ/mol O2`:

| T (°C) | Model $\mu_{O_2}^{rel}$ | O'Neill | Delta |
|---:|---:|---:|---:|
| 570 | -421804.8 | -417188.1 | -4616.6 |
| 575 | -420646.2 | -416016.9 | -4629.3 |
| 600 | -414727.2 | -410143.9 | -4583.3 |

So the O'Neill comparison localizes the main remaining error very
cleanly:

- $\mathrm{Fe|wustite}$ is essentially right
- $\mathrm{wustite|spinel}$ is still shifted

### 5.3 Tier 3 Fe-O-H validation: Spreitzer/Schenk

The earlier severe mismatch turned out to be a false alarm:

- the first traced curves used for comparison were from the
  `Fe-O-C` diagram, not the `Fe-O-H` diagram

After switching to the corrected $\mathrm{H_2}$-based traced lines, the
Tier 3 picture improved substantially.

The current comparison plot is:
[bg_compare_all_h2_helmholtz_plus_ref0.png](res/bg_compare_all_h2_helmholtz_plus_ref0.png)

![Corrected $\mathrm{H_2}$ boundary comparison](res/bg_compare_all_h2_helmholtz_plus_ref0.png)

This plot compares the current FPROPS Tier 3 curves against the
corrected $\mathrm{H_2}$-based traced lines for:

- $\mathrm{Fe|wustite}$
- $\mathrm{wustite|spinel}$
- $\mathrm{Fe|spinel}$

Assessment:

- overall agreement is now good
- $\mathrm{Fe|wustite}$ is especially good
- $\mathrm{Fe|spinel}$ is reasonable
- the main visible residual discrepancy remains
  $\mathrm{wustite|spinel}$

That level of agreement is good enough to proceed with Tier 4 and
other Tier 3-based work.

## 6. What Was Fixed And Should Stay Fixed

These items were genuine bugs or design gaps and should be treated
as resolved unless new evidence appears.

### 6.1 Chemistry-compatible gas reference states

The gas-side reference-state handling is now substantially better:

- native Helmholtz fluids carry real `ref0` chemistry anchors
- `helmholtz+ref0:` is usable for equilibrium chemistry
- Moran and Shapiro ideal-gas files were corrected
- the RPP path was corrected so ideal-gas heat capacities and
  chemistry anchors are interpreted consistently

### 6.2 Wustite excess term

The Hidayat excess term is implemented as the direct polynomial
given by the model, not as a Redlich-Kister surrogate.

This correction was essential.

### 6.3 Fe-side allotropic consistency

The pure-iron side and magnetic treatment were corrected enough to
place the `bcc/fcc` transition in the right region. This is no
longer the main source of error.

## 7. Remaining Gaps

This is the short list that still matters.

### 7.1 Reduced spinel model

The dominant remaining gap is the reduced Fe-only spinel
approximation.

What is true now:

- the current model already captures the qualitative spinel behavior
- the `Fe3O4` endmember has been updated to the Hidayat 2015 value
- but the reduced model still leaves the $\mathrm{wustite|spinel}$ boundary
  too oxidized

What is not yet implemented:

- the full spinel treatment inherited by Hidayat from Ref. [8]
- full consistency of the spinel CEF with the final Fe-O assessment

### 7.2 Invariant temperature placement

Because the oxide-side boundary is still shifted, the common
invariant remains slightly too hot in the current model.

This is not a blocker for proceeding, but it should remain visible
in interpretation of oxide-ladder results.

### 7.3 Plotting and diagnostics

The comparison harness remains useful, but should be treated as a
diagnostic script rather than the thermodynamic source of truth.

The relevant file is:
[feoh_baur_glaessner_compare.py](/home/john/ascend/models/johnpye/fprops/test/feoh_baur_glaessner_compare.py)

## 8. Current Output Files

The most useful current artifacts are:

- corrected $\mathrm{H_2}$ comparison plot:
  [bg_compare_all_h2_helmholtz_plus_ref0.png](res/bg_compare_all_h2_helmholtz_plus_ref0.png)
- Tier 3 boundary script:
  [feoh_hydrogen_boundary.py](/home/john/ascend/models/johnpye/fprops/test/feoh_hydrogen_boundary.py)
- Spreitzer comparison harness:
  [feoh_baur_glaessner_compare.py](/home/john/ascend/models/johnpye/fprops/test/feoh_baur_glaessner_compare.py)

## 9. Recommended Next Work

Recommended priority:

1. Proceed with Tier 4 `Fe-O-C-H` using the current Tier 3 model.
2. Continue to treat the `wustite|spinel` side as the main residual
   uncertainty.
3. Return to full spinel refinement only when that oxide-side shift
   becomes the dominant limitation for the next task.

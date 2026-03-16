## UNIFAC Validation

This note records the recommended validation ladder for the current
ideal-vapor + UNIFAC-liquid flash implementation in FPROPS.

### Scope

The current kernel solves a fixed-`T`, fixed-`P`, fixed-`z` vapor-liquid
flash for mixtures whose vapor phase is treated as ideal and whose liquid
phase is treated with the original UNIFAC equations currently implemented
in `thermodynamics.a4l`.

Validation therefore needs to distinguish between:

1. implementation correctness
2. agreement with the legacy ASCEND thermodynamic models
3. agreement with experiment

These are not the same question.

### Recommended Ladder

#### 1. Internal consistency against ASCEND thermodynamics

Use `fprops_unifac_flash_state` and compare the solved `x` and `y` against
the existing `ideal_vapor_mixture` and `UNIFAC_liquid_mixture` models by
checking equality of partial Gibbs energies.

This is currently the strongest automated check because it verifies that
the new C kernel reproduces the same thermodynamic basis already used in
ASCEND.

Current status:

- `test_fprops_unifac_blackbox_ethanol_water` passes
- `test_fprops_unifac_phase_eq_ethanol_water` passes

See `unifac_flash_demo.a4c`.

#### 2. Pointwise activity-coefficient checks against original UNIFAC

Use the public DDBST original-UNIFAC estimator as an external spot check
for `gamma_i(T, x)`.

This is the right check for the original UNIFAC implementation itself,
because it avoids confusing solver problems or model-fit error with coding
error.

Source:

- DDBST property estimation page:
  https://www.ddbst.com/prp-estimate.html
- direct ethanol-water calculation page at `298 K`:
  http://ddbonline.ddbst.com/UNIFACCalculation/UNIFACCalculationCGI.exe?component1=Ethanol&component2=Water&temperatures=298&calculate=Calculate

The exact ethanol-water table from that public DDBST page is stored in:

- `test/ethanol_water_gamma_ddbst_298K.csv`
- benchmark harness:
  `test/ethanol_water_unifac_gamma_ddbst_benchmark.py`

Current exact DDBST spot-check status:

- `test_fprops_unifac_gamma_ddbst_ethanol_water` checks the midpoint
  `x_ethanol = 0.5` values:
  - `gamma_ethanol = 1.2036`
  - `gamma_water = 1.4968`

Example command:

```bash
python3 models/johnpye/fprops/test/ethanol_water_unifac_gamma_ddbst_benchmark.py
```

Example regression guard:

```bash
python3 models/johnpye/fprops/test/ethanol_water_unifac_gamma_ddbst_benchmark.py \
  --assert-combined-rms-max 5e-5 \
  --assert-combined-max-max 6e-5
```

Current baseline for the present FPROPS implementation:

- `gamma_ethanol` RMS absolute error: `0.000031`
- `gamma_ethanol` maximum absolute error: `0.000049`
- `gamma_water` RMS absolute error: `0.000025`
- `gamma_water` maximum absolute error: `0.000044`
- combined RMS absolute error: `0.000028`
- combined maximum absolute error: `0.000049`

#### 3. Experimental VLE at atmospheric pressure

For physical validation, use the binary ethanol-water VLE data reported at
`101.3 kPa` in:

- Liu, Lei, Wang, Li, Zhu, "Isobaric Vapor-Liquid Equilibrium for the
  Ethanol + Water + 2-Aminoethanol Tetrafluoroborate System at 101.3 kPa",
  *J. Chem. Eng. Data* 2012, 57, 3532-3537
  https://doi.org/10.1021/je3007138

Table 1 of that paper reproduces the binary ethanol-water data with stated
composition and temperature uncertainties. A copy of those binary points is
stored in:

- `test/ethanol_water_vle_101kPa_liu2012.csv`
- benchmark harness:
  `test/ethanol_water_unifac_tpz_benchmark.py`

For the current `TPz` flash kernel, a practical way to use these data is:

1. take an experimental tie-line point `(T, x, y)` at `101.3 kPa`
2. choose any vapor fraction `beta` between `0` and `1`
3. construct `z = beta * y + (1 - beta) * x`
4. solve `TPz`
5. compare the returned `x` and `y` against the experimental values

Using `beta = 0.5` is sufficient for this benchmark.

Current baseline for the present FPROPS implementation:

- `x_ethanol` RMS absolute error: `0.064794`
- `y_ethanol` RMS absolute error: `0.028463`
- combined RMS absolute error: `0.050042`
- combined maximum absolute error: `0.106367`

Example command:

```bash
python3 models/johnpye/fprops/test/ethanol_water_unifac_tpz_benchmark.py
```

Example regression guard using the current baseline:

```bash
python3 models/johnpye/fprops/test/ethanol_water_unifac_tpz_benchmark.py \
  --assert-combined-rms-max 0.051 \
  --assert-combined-max-max 0.107
```

This test is useful, but it must not be interpreted as a pure
implementation check. Ethanol-water is strongly associating, so residual
error here includes model error from original UNIFAC itself.

#### 4. Azeotrope checks

Use the more recent azeotrope data reported in:

- Gil et al., "High-Pressure Phase Equilibria in an Ethanol/Water Binary
  System: Experimental Data and Modeling", *J. Chem. Eng. Data* 2020
  https://doi.org/10.1021/acs.jced.0c00686

Accessible values from the abstract:

- at `100 kPa`: `x_az = y_az = 0.899`, `T = 351.39 K`
- at `1500 kPa`: `x_az = y_az = 0.857`, `T = 440.70 K`
- at `2000 kPa`: `x_az = y_az = 0.850`, `T = 453.83 K`

These are useful as compact regression anchors once higher-pressure flash
support exists.

### Interpretation Guidance

For ethanol-water, the correct validation sequence is:

1. match ASCEND `UNIFAC_liquid_mixture`
2. match original UNIFAC spot values from DDBST
3. compare to experiment

Any mismatch at step 3 is not automatically a coding defect.

### Current Limitation

Direct comparison against the legacy `flash.a4l` `vapor_liquid_flash` model
for fixed-`T`, fixed-`P` ethanol-water states is still limited by solver and
formulation issues in that older wrapper. The new FPROPS kernel should
therefore currently be validated against:

- the ASCEND phase models directly
- external UNIFAC spot checks
- external experimental VLE data

rather than treating `flash.a4l` as the reference implementation.

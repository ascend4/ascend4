# Part I reproduction benchmark

**Original target:** Smith and Westerberg (1991), *The optimal design of
pressure swing adsorption systems*, Chemical Engineering Science 46,
2967–2976, DOI [10.1016/0009-2509(91)85001-E](https://doi.org/10.1016/0009-2509(91)85001-E).
Local source: `~/Downloads/1-s2.0-000925099185001E-main.pdf`.

This benchmark separates three things:

1. Immutable, dimensioned **published inputs and outputs** from Part I.
2. Algebraic **consistency checks on those published numbers**.
3. **Conditional operation calculations** using the implemented printed
   equations and explicitly identified assumptions.

It does not perform design optimisation or assemble a Part I closed cycle.
A separate [closed zero-PE journal-equation audit](psa_part1_cycle.md) now
connects those operations and reports pressure and energy-accounting
discrepancies. The thesis and Part II values are not substituted into this
source deck.

The closed-cycle audit also now includes [inverse diagnostics and a
rounding-aware adsorption heat check](psa_part1_cycle.md#inverse-diagnostics-can-the-missing-k-explain-the-mismatch).
Under the stated zero-PE adsorption/inventory interpretation, the Cen–Yang
20.92 kJ/mol heat is incompatible with the reported flows even allowing for
their printed rounding and that of the geometry and pressure. This check
does not depend on K or the added thermal interface rules; it identifies
a reconstruction-assumption conflict, not a recovered alternative heat.

[Conservative alternatives](psa_part1_physical.md) now test different
thermal/pressure and purge-equilibrium assumptions at the same Part I
zero-PE design point, without fitting parameters. Their isothermal limit
is within about 6.5% of the reported main flows, with explicit heating and
cooling requirements; this is not yet design optimisation.

```sh
./a4 run models/psa/psa_part1.a4c
./a4 script models/psa/psa_part1.py
./a4 script models/psa/psa_part1.py --json
./a4 script models/psa/psa_part1.py --adsorption-heat 18000
./a4 script models/psa/test/test_psa_part1.py
```

The [ASCEND model](psa_part1.a4c) owns the source data; the
[Python report](psa_part1.py) reads it through ascpy. JSON quantities have
explicit unit suffixes. Missing PE pressure points are JSON `null`, not
physical zero-pressure states. No Gurobi licence is needed for these QRSlv
equation checks.

## Source deck and interpretation

Tables 1 and 4 were checked against scanned pp. 2972 and 2974, not just
OCR. The data component includes the four configurations, bed counts,
cycle integers, geometry, utilisation, pressure points, slot durations,
batch amounts, recovery and cost targets. Table 3 costs and the more
precise Table 5 baseline costs are both retained.

| PE steps | Beds | Adsorption utilisation | d (m) | L (m) | P9 (international psia) | D (s) | Table 5 cost (MUSD/year) |
|---|---:|---:|---:|---:|---:|---:|---:|
| 0 | 2 | 0.75 | 0.45 | 2.24 | 225.0 | 54.8 | −1.3292 |
| 1 | 3 | 0.85 | 0.40 | 1.98 | 356.0 | 64.7 | −1.3901 |
| 2 | 4 | 0.90 | 0.39 | 1.98 | 358.7 | 70.1 | −1.3964 |
| 3 | 5 | 0.95 | 0.40 | 1.97 | 360.5 | 74.2 | −1.3953 |

The cost column is a **published target**, not an evaluation of implemented
economics. It is Table 5 at product price 3.5 USD/kgmol, in the paper's
historical cost basis; no inflation update is applied.

Table 1 inputs include Ffeed = **31.1 mol/s**, rather than the thesis's
31.11; both efficiencies are 0.95, and the desorbed fraction is 1.0.
Pressure conversion uses 6894.757293168 Pa per international psi, without
altering ASCEND's legacy global `psi` unit.

The four-digit batch quantities in Table 4 are interpreted as **mol**,
despite the general nomenclature's kgmol convention. This is an explicit
unit interpretation: only this scale approximately agrees with the stated
feed rate and scheduling shift. The printed 332.2 psia value for P2 in the
two-PE column is preserved, not silently changed to a more expected value.

Table 4 gives tau1–tau5 and tau9. The missing reciprocal PE slots
tau6–tau8 are inferred from equal-duration coupling; they are identified
as derived values. Absent operations have zero slots. Full cycle duration
is N × D. The implementation checks slot sums and reciprocal start-time
separations against these values, allowing for printed rounding.

## Findings from the published numbers alone

### Recovery: equation (31) does not reproduce the listed recoveries

```text
recovery = (gross_H2 − purge_H2) / [(1 − yfeed) × (adsorption_feed + repressurisation_feed)]
```

| PE steps | Printed recovery | From Table 4 amounts and eq. (31) |
|---|---:|---:|
| 0 | 0.90 | 0.916671 |
| 1 | 0.95 | 0.971229 |
| 2 | 0.96 | 0.979546 |
| 3 | 0.97 | 0.984477 |

All four discrepancies exceed the possible rounding intervals from the
displayed last digits: ±0.5 mol for integer amounts, ±0.05 mol for
one-decimal amounts, and ±0.005 for recovery. The feed composition is
treated as the specified value 0.05, not a rounded measurement.

This is a **source-consistency finding**, not proof of the cause. Unreported
dead-volume losses or different stream boundaries could be relevant, but
no such adjustment is inferred or fitted here. Published recovery and
amount-derived recovery remain separate fields.

### Throughput: a factor of the bed count

Equation (37) on p. 2972 prints:

```text
Ffeed × total_cycle_time = adsorption_feed + repressurisation_feed
```

The Table 4 amounts instead agree, within about 0.09%, with
`Ffeed × D`, where equation (1) defines `D = total_cycle_time/N`.
Equivalently, the usual plant-wide balance would be:

```text
Fplant × total_cycle_time = N × feed_per_bed_cycle
```

Thus a per-bed/plant feed convention or a missing factor N needs resolution
before using the literal printed equation in optimisation. The report
shows both ratios; it does not edit the source equation.

### Adsorption slots are not always equal to D

For zero PE, tau1 = 93.1 s while D = 54.8 s; for one PE, tau1 = 70.1 s
while D = 64.7 s. The source's continuous-production constraint on p. 2972
allows actual adsorption duration at least D, rather than exactly D.

Our earlier timing LP imposed both actual and allocated adsorption
duration equal to D. It therefore cannot represent these tabulated slots
unchanged. A Part I scheduler must allow the more general timing condition
and distinguish processing time from standby. The table alone does not
identify the split into those two quantities.

## Conditional operation comparisons

For each Table 4 geometry, the benchmark evaluates the existing printed
adsorption/desorption components with:

- P9 used as adsorption pressure, and P5 as purge pressure;
- the Part I utilisation and efficiency factors;
- initially uniform adsorption-bed temperature 298 K and initial solid
  methane loading zero;
- post-blowdown temperature independently specified as 298 K;
- adsorption heat 20920 J/mol from Cen–Yang, **as a hypothesis**;
- the candidate infinite-dilution mass-loading slope for the journal's K;
- desorption reference loading taken at the computed adsorption equilibrium.

The last five choices are not a recovered Part I cyclic solution. In
particular, zero initial solid loading excludes methane captured during
the preceding repressurisation, and neither post-blowdown temperature nor
the operation interfaces are solved here. The label `pe3` selects the
published three-PE **design point**, not a simulated three-PE cycle.

| PE steps | Adsorption feed: calculated / printed (mol) | Gross H2: calculated / printed (mol) | Purge H2: calculated / printed (mol) |
|---|---:|---:|---:|
| 0 | 1443.18 / 1610 | 1377.34 / 1533 | 113.93 / 47.7 |
| 1 | 1584.01 / 1953 | 1513.88 / 1861 | 85.98 / 4.5 |
| 2 | 1602.60 / 2137 | 1531.68 / 2035 | 88.76 / 4.5 |
| 3 | 1776.54 / 2272 | 1697.96 / 2164 | 100.62 / 4.5 |

These discrepancies are reported, **not asserted to be solver failures**.
They show that the present assumptions do not reproduce the published
operation quantities. In particular the Henry-slope hypothesis should not
be mistaken for an established interpretation of K. No cycle recovery
is calculated from these incomplete operation results.

`--adsorption-heat` changes only the conditional calculations. Published
targets remain compiler constants. The Table 1 heat values of 890000 and
286000 J/mol are retained under explicitly separate names, without claiming
that they are suitable adsorption heats. Extreme assumptions may not have
a feasible solution from the default initial point; solver failure is
reported, not hidden behind a substitute result.

## Next work toward Part I, not Part II

1. The [zero-PE journal assembly](psa_part1_cycle.md) now implements the
   pressure-changing analogues and cyclic interfaces, with explicit
   pressure conventions and energy-defect diagnostics. Resolve or quantify
   those discrepancies before treating it as a physically feasible design;
   do not silently substitute the thesis purge or conservative heat equations.
2. Use the present fixed-design comparisons to diagnose K, heat and stream
   conventions. Where the source remains incomplete, distinguish a literal
   equation audit from a documented reconstruction scenario.
3. Implement cost correlations, feed compression, utilities, velocity and
   extra-column constraints, with unavailable inputs exposed explicitly.
4. Extend to one, two and three PE configurations and optimise each at
   initially fixed scheduling integers. Only then compare costs and designs
   with Tables 3–5 and broaden the scheduling search.

The [conservative thermal cycle](psa_thermal_cycle.md) remains a useful
independent physical-accounting reference, not a replacement for the Part I
target. Dynamic purity validation belongs to the later thesis/Part II
programme and is not a prerequisite for this source-reproduction work.

Seven regression tests cover transcription, units, source inconsistencies,
scheduling/throughput conventions, independent scalar operation roots,
assumption/source separation, and text/JSON reporting. They are included in
the PSA CI step. Passing these tests validates the benchmark machinery,
**not the historical optimisation result**.

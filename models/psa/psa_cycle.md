# Fixed-design PSA cycles and conditional scheduling

These models close the methane and hydrogen balances of **isothermal,
zero- and one-pressure-equalisation cycles**. They are stepping stones towards the
Smith–Westerberg design optimisation, not a reproduction of its optimum.

```sh
./a4 run models/psa/psa_cycle.a4c
./a4 script models/psa/psa_cycle.py
./a4 script models/psa/psa_cycle.py --equalisations 1
./a4 script models/psa/test/test_psa_cycle.py
```

The first command solves with QRSlv and checks balances. The Python script
prints labelled inventories, per-bed batch transfers, and plant-average
rates in SI units. No commercial license is required. The regression tests
also use IPOPT when available, with a test-only objective to satisfy that
adapter: all physical inputs remain fixed, so this is a feasibility check,
not design optimisation. For one PE, IPOPT is initialised using a QRSlv
solution at 250 psi, then solves the changed problem at 355 psi; generic
atom defaults were not a reliable starting point for IPOPT.

## Physical structure

Without PE, one bed passes through four operations; identical beds repeat
the same cycle.

```text
pressurised → adsorption → loaded → blowdown → blown_down
     ↑                                             ↓
feed repressurisation ← regenerated ← hydrogen purge
```

`psa_isothermal_bed` owns geometry, void fraction, carbon density and imposed
temperature. Each `psa_bed_state` references that bed and owns three separate
inventories: gas-phase methane, gas-phase hydrogen, and adsorbed methane.
Each operation references its start and finish states. Feed repressurisation
returns to the **same state instance** from which adsorption starts.

`WHERE` conditions require the two states of an operation to belong to the
same bed. Adsorption and purge require shared pressure variables, and the
adsorption isotherm shares bed temperature. Those identities are established
by the containing model with `ARE_THE_SAME`; the operations do not mutate
the types or identities of their supplied parameters.

The one-PE case inserts a donor event between adsorption and blowdown, and
a receiver event between purge and feed repressurisation. A single
`psa_equalisation_step` connects all four endpoint states and transfers
CH4 and H2 internally. The representative cycle describes identical beds
at different phases: the donor and receiver are distinct physical beds,
although they share the same geometry/temperature model instance here.
The component also supports distinct bed designs, tested separately.

`psa_cycle_base` owns the common operations and accounting. Derived cases
select the boundary set and the blowdown/repressurisation inlet states via
`ALIASES`, then add their topology-specific operations. There are 41 free
variables and equations for zero PE, and 53 for one PE, after `on_load`
fixes the design/operating inputs. Plant-wide balances are independent
`self_test` assertions, not redundant extra equations.

## Explicit approximations

The operation models are inspired by
[Smith and Westerberg (1991)](https://doi.org/10.1016/0009-2509(91)85001-E),
especially equations (6)–(25) and (28)–(31). Isothermal operation follows the
dynamic-model assumption in their
[1992 follow-up](https://doi.org/10.1016/0009-2509(92)85170-G).
The following choices prevent this from being an exact transcription:

| Operation | Closure used here |
|---|---|
| Adsorption | A sharp front uses a specified bed fraction; that fraction contains feed-composition gas, with pure H2 in the remaining voids. Carbon loading comes from the feed isotherm. Product is assumed pure H2. |
| Pressure equalisation | Donor solid loading is frozen and donor void gas is well mixed. The receiver captures incoming CH4 on carbon and retains transferred H2 as gas. Both final pressures are equal. This is an isothermal analogue of 1991 eqs. (14)–(25), not the paper's adiabatic transfer and subsequent gas/solid thermal equilibration. |
| Blowdown | Solid loading is frozen. Void gas is treated as well mixed, so its composition remains unchanged as pressure falls. This is an explicit reconstruction choice. |
| Purge | Remove 98% of adsorbed methane and end with pure H2 in the voids. Supply 1.25 mol H2 per mol **solid-phase** CH4 removed. Component balances determine the exhaust, including the initial void-gas methane. |
| Feed repressurisation | Capture all incoming methane on carbon and retain incoming hydrogen in the voids; no outlet. This is the isothermal form of the assumption accompanying 1991 equation (30). |

The purge ratio is motivated by the discussion on p. 4215 and Table 4 of
the 1992 paper, but its definition here is explicit and is **not presented
as equation (26)** from 1991. The 98% regeneration and 1.25 ratio are
illustrative choices borrowed from the one-equalisation iteration table;
they are not established zero-equalisation calibration data.

There are no energy balances, heat-transfer or mass-transfer rates, pressure
drop, valve characteristics, intraparticle gas inventory, extra-column dead
volume, or predictions of operation durations. Isothermal operation presumes
whatever heat exchange is required; utility duties have not been calculated.
Residual adsorbate and gas inventories are cyclic, but no dynamic trajectory
has been integrated to demonstrate that this state is attainable or stable.
The specified front utilization does not establish product purity.

## Data and current result

Geometry, temperature and pressures are drawn from the 1992 tables:
diameter 0.396 m, length 1.982 m, temperature 299 K, adsorption pressure
355 international psi and regeneration pressure 15 international psi.
The component converts those pressures explicitly to Pa; see
[the property documentation](psa_properties.md).

The baseline selects void fraction 0.44 from Table 1 rather than 0.4 from
Table 2; carbon particle-envelope density is 800 kg/m³, and plant feed is
31.11 mol/s with 5% methane. Dry carbon mass is `(1 − eps) × rho × V`,
not `rho × V`: the density is not interpreted as bulk packed-bed density.
Adsorption bed utilization is 0.75 (Table 5's zero-equalisation case) and
the equilibrium loading multiplier is 1. **Both cases retain those same
values**, to isolate the effect of PE; this does not adopt the table's
one-PE utilisation of 0.78.

| Quantity | Zero PE, two beds | One PE, three beds |
|---|---:|---:|
| Carbon mass per bed | 109.360860 kg | 109.360860 kg |
| Feed repressurisation per bed per cycle | 106.611855 mol | 54.324512 mol |
| Internal H2 purge per bed per cycle | 110.639744 mol | 110.639744 mol |
| Net H2 product per bed per cycle | 1549.115263 mol | 1562.017335 mol |
| Net hydrogen recovery | 88.164654% | 90.799137% |
| Throughput-implied cycle period | 118.903765 s | 174.623129 s |
| Plant-average net H2 product | 26.056623 mol/s | 26.835231 mol/s |

One PE transfers 1.935311 mol CH4 and 49.672976 mol H2, internally, per
bed per cycle. The equalised pressure is 12.531331 bar absolute, below the
arithmetic mean of the high and low pressures. Capturing CH4 removes it
from the gas phase. For identical isothermal beds, receiver initially
containing pure H2, and donor methane gas fraction x, conservation gives
`P_PE = [P_low + (1 − x) P_high] / (2 − x)`. Only when x = 0 is the
arithmetic mean appropriate. The component tests also cover unequal gas
volumes/temperatures and initial receiver gas-phase methane.

Internal purge is deducted from gross product, and **both** adsorption feed
and repressurisation feed enter the recovery denominator (1991 eq. 31).
At the external boundary, all incoming methane leaves as waste and all
incoming hydrogen leaves as either net product or waste.

The cycle period is inferred from `Ffeed × period = nbed × feed_per_bed`.
It is average throughput accounting, not a kinetic limit or a validated
schedule. It does not imply constant instantaneous feed demand or establish
the required peak feed/compressor capacity.

## Conditional timing LP

The optional scheduling model follows the fixed-topology linearisation in
[Smith and Westerberg (1990)](https://doi.org/10.1016/0009-2509(90)80176-F),
eqs. (2), (5), (6), (8), (10). It uses **externally specified processing
times** and the throughput-implied period; it does not derive valve,
purge or mass-transfer rates.

- Cycle period = N × D, where D is the shift between successive beds.
- Each allocated operation slot = actual processing time + standby.
- Actual adsorption and its allocated slot both last D, giving exactly
  one adsorbing bed throughout the cycle. Standby is never credited as
  production.
- For one PE, N = 3 and J = 1: PE-up starts D after PE-down on the
  representative bed, so the donor on bed b meets the receiver on bed b−1,
  including across the cycle boundary. Paired actual durations must match.
  Equal *allocated* PE slots are an additional symmetric-layout restriction,
  not a general physical requirement.
- An additional unbuffered-product assumption requires gross H2 production
  during purge to cover purge demand: `gross × purge_time ≥ purge_use × D`.
  This assumes uniform production during adsorption and uniform purge use.
  These topologies have at most one purging bed at a time.

The LP maximises the minimum non-adsorption standby allowance, purely to
choose a layout. It does **not** minimise costs, choose a bed count, search
alternative values of J, or establish physically achievable processing
times. The Python driver independently checks the returned layout and
cross-bed synchronisation.

For example, to exercise the LP with **illustrative, uncalibrated** times:

```sh
./a4 script models/psa/psa_cycle.py --equalisations 1 --timing \
    --blowdown-time 1 --purge-time 31 --repressurisation-time 1 \
    --equalisation-time 1
```

This gives D = 58.207710 s and minimum standby = 8.402570 s. The 1 s
pressure-change times are test inputs, **not** predictions or recommendations
for this bed. HiGHS is the default; `--timing-solver Gurobi` selects Gurobi
when built and licensed. All relevant times must be supplied explicitly.

The distinction between overall duration and synchronisation matters:
with PE = 1 s, blowdown = 25 s, purge = 40 s and repressurisation = 1 s,
total non-adsorption processing fits the available 2D, but PE-down +
blowdown + purge exceeds D. No layout exists under this topology.
Conversely, a 1 s purge fits the clock but cannot be supplied with H2
under the no-buffer assumption. Both failures are regression-tested.

### Comparison with the paper is not yet a validation

The zero-equalisation column of 1992 Table 5 prints recovery 0.86. Its
listed batch amounts, however, give

```text
(1622 − 115.5) / [0.95 × (1703 + 103.2)] = 0.877970...
```

when inserted directly in 1991 eq. (31). That discrepancy is not explained
by rounding to two decimal places. We preserve both published facts rather
than force a fitted match. Neither value is used as an expected result in
the regression tests. Adsorption heat conventions and the other table
discrepancies remain unresolved as documented in `psa_properties.md`.

## Tests and next steps

The tests independently eliminate the cycle balances into scalar formulas,
compare all boundary inventories and transfers, check every operation's
component balances, and sweep 16 pressure/temperature/composition cases.
Both cycles are swept over 16 operating conditions. Tests also check
dimensions, geometry scaling, plant-feed scaling, the coupled PE event
with unequal beds, and feasible/infeasible scheduling. HiGHS is required
for timing tests; Gurobi and IPOPT are exercised when registered, without
a separate opt-in environment variable.

A useful consequence of the scaling test: doubling bed volume doubles batch
amounts and cycle period at fixed plant feed, but leaves recovery unchanged.
That is expected for this simplified model. It also shows why optimising bed
size **before** adding timing, transport and design limits would be misleading.

Next: derive or calibrate the processing times and desorption/thermal
closures, extend to more PE stages where justified, then add economics
and fixed-configuration design NLPs. The conditional LP now checks timing
consistency, but does not remove the need for transport and equipment
limits. Dynamic purity verification remains a separate later stage.

The [thermal equation audit](psa_thermal.md) now implements and tests the
1991 adsorption/desorption energy equations separately. It records the
unresolved heat, equilibrium-constant and initial-void-gas conventions
that must be addressed before replacing these closed isothermal cycles.

Local paper references:

- `~/Downloads/1-s2.0-000925099185001E-main.pdf` (1991 design).
- `~/Downloads/1-s2.0-000925099285170G-main.pdf` (1992 verification).
- `~/Downloads/1-s2.0-000925099080176F-main.pdf` (1990 scheduling).
- `~/Downloads/cen-1986-psa.pdf` (adsorption data).

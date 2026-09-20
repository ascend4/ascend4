# Closed zero-equalisation thermal PSA cycle

This is a **fixed-design, energy-conserving reconstruction**, not a
reproduction of the published optimum or a literal implementation of all
the thesis's approximate heat equations. The temperature and adsorbed
inventory at the start of adsorption are now determined by cyclic closure,
rather than independently specified operation inputs.

```sh
./a4 run models/psa/psa_thermal_cycle.a4c
./a4 run models/psa/psa_thermal_cycle.a4c -p recovery product heater period
./a4 run models/psa/psa_thermal_cycle.a4c -m psa_thermal_cycle_isothermal
./a4 script models/psa/test/test_psa_thermal_cycle.py
```

The components are in [psa_thermal_cycle.a4l](psa_thermal_cycle.a4l), the
assembled example in [psa_thermal_cycle.a4c](psa_thermal_cycle.a4c), and the
tests in [test_psa_thermal_cycle.py](test/test_psa_thermal_cycle.py).
QRSlv solves the default example directly. IPOPT is cross-checked from a
QRSlv solution at a neighbouring pressure; its test objective is only a
feasibility-test device, not an economic objective. Gurobi and HiGHS cannot
solve these nonlinear equations through their current linear adapters.

## Sources and boundaries of the reconstruction

Principal source: Oliver Jacob Smith IV, *The Optimal Design of Pressure
Swing Adsorption Systems*, CMU PhD thesis, July 1991, Chapter 6, printed
pp. 80–88; example data in Tables 7-1, 7-10 and 7-11.
Local PDF: `~/Downloads/smith-1991-The_optimal_design_of_pressure.pdf`.

The previous [thermal-equation audit](psa_thermal.md) preserves the printed
journal and thesis heat equations, including their limitations. It is
unchanged by this new cycle. Likewise the earlier [isothermal mass
cycles](psa_cycle.md) retain their original purge-supply convention.

This cycle retains the thesis's two-zone adsorption approximation,
utilisation and loading factors, frozen-solid blowdown, complete methane
capture during feed repressurisation, and empirical purge supply equation
(6.22). It **replaces the approximate heat balances** with one consistent
inventory/flow-energy convention. The following closures are ours, not
additional information recovered from the historical implementation:

| Operation | Thermal/inventory closure |
|---|---|
| Adsorption | Hot region occupies 75% of the bed at feed composition and equilibrium loading; cold tail remains at initial temperature, with pure H2 and no adsorbed CH4. All adsorbed methane is assigned to the hot region. Product leaves at cold-tail temperature. Complete initial/final internal-energy accounting replaces thesis eq. (6.6). |
| Internal mixing | Sealed, adiabatic equilibration of the two regions with frozen solid loading. All component inventories and internal energy are conserved. Temperature and pressure are calculated; pressure is **not** reset to the adsorption pressure. |
| Blowdown | Well-mixed gas withdrawal with frozen solid loading and instantaneous gas/solid thermal equilibration. This differs from the thesis's gas-only adiabatic expansion followed by separate thermal equilibration. |
| Purge | Thesis eq. (6.22), including the final void H2 charge, plus complete gas/solid balances. Both exhaust components use an effective temperature Tout = (1 − beta) Tinitial + beta Tfinal; beta defaults to 0.5 and is an explicit approximation. Complete inventory/stream energy replaces thesis eq. (6.21). |
| Feed repressurisation | No outlet; all entering CH4 is adsorbed, H2 fills the voids. The full rigid-bed filling energy balance includes adsorption heat and inlet enthalpy. |

The end of feed repressurisation is the **same state instance** as the
start of adsorption. There is no independently specified starting bed
temperature or loading. Internal mixing is a zero-duration state-mapping
approximation here, not a newly scheduled operating step.

## Caloric convention

Gas is ideal, heat capacities are constant, the solid/gas in each region
share a temperature, and adsorbed CH4 has negligible PV energy. We assume
the adsorbed CH4 heat capacity equals its gas cp; this is an explicit
closure, not a separately sourced adsorbed-phase property.

For each component and a chosen reference temperature Tref:

```text
h_gas = cp × (T − Tref)
u_gas = h_gas − R × T
u_adsorbed_CH4 = cp_CH4 × (T − Tref) − Hads
U_carbon = Csolid × (T − Tref)
```

Here Hads is a positive adsorption-enthalpy release magnitude. In
particular, `u_gas` is **not** `(cp − R) × (T − Tref)` when the above
enthalpy zero is used. That alternative would introduce an inconsistent
offset between gas and adsorbed methane. Tests shift Tref without changing
the predicted physical temperatures, amounts or heat duties.

Default Hads is the Cen–Yang value 20920 J/mol. The thesis's ambiguous
890/286 kJ/mol table values are not used as adsorption heats. The empirical
LRC and this independently chosen caloric model are not claimed to form a
thermodynamic-potential-consistent adsorption-property package. Bed shell
heat capacity, heat losses and extra-column gas volumes are omitted.

For every operation, the common energy convention is:

```text
Ufinal − Uinitial = incoming stream enthalpy − outgoing stream enthalpy + Q
```

Q is positive into the bed. It is zero in the default adiabatic model.
Internal energies and stream enthalpies may be negative relative to Tref;
they therefore use signed dimensional energy variables.

### Blowdown endpoint law

With constant gas composition, let cv be its molar constant-volume heat
capacity and S = Csolid + adsorbed_CH4 × cp_CH4. Integrating the rigid-bed
energy balance gives:

```text
ln(Tfinal/Tinitial) = (R/cv) × ln((S + nfinal cv)/(S + ninitial cv))
```

The ideal-gas relation supplies nfinal at the specified low pressure.
An independent quadrature test integrates outlet enthalpy over the
withdrawn gas and compares it with the calculated internal-energy loss.

### Purge and the external plant boundary

Purge H2 is taken from gross product, not purchased as another feed stream:

```text
purge_H2 = 1.25 × desorbed_CH4 + final_void_H2
net_product = gross_product − purge_H2
purge_heater = purge_H2 × cp_H2 × (Tpurge − Tproduct)
```

Initially present void gas exits in the purge exhaust; it is neither
deleted nor subtracted a second time from the supplied amount. Over the
whole cyclic bed, the independent external balance is:

```text
feed_enthalpy + purge_heater + sum(bed_heat_duties)
    = net_product_enthalpy + blowdown_enthalpy + purge_exhaust_enthalpy
```

The heater is an energy **per bed-cycle**, not a power. Multiply it by
2/period for the average two-bed duty. At the default point it supplies
approximately 128.454 kJ/bed-cycle, or 2.834 kW for the two-bed throughput.
Feed arrives at its specified pressure and temperature: upstream feed
compression and its cost are outside this boundary.

## Fixed-design baseline

Geometry is d = 0.396 m, L = 1.982 m; eps = 0.44, dry carbon density
800 kg/m³. High/low pressures are 355/15 international psi absolute
(2.447639/0.103421 MPa). Feed is 5 mol% CH4 in inert H2 at 298 K and
31.11 mol/s plant average. Purge enters at 350 K. Adsorption utilisation
is 0.75, equilibrium efficiency 1, and regenerated solid fraction 0.98.

These inputs are a declared reconstruction scenario. The following
numbers are **new calculated results, not published benchmarks**:

| Quantity | Result |
|---|---:|
| Temperature after feed repressurisation / start of adsorption | 300.128648 K |
| Hot adsorption-region temperature | 313.922342 K |
| Temperature after sealed internal mixing | 310.547989 K |
| Temperature after blowdown | 308.337541 K |
| Temperature after purge | 296.893783 K |
| Mixed pressure before blowdown | 2.449150 MPa |
| External feed per bed-cycle | 1410.070017 mol |
| Gross H2 product per bed-cycle | 1245.963589 mol |
| H2 supplied to purge per bed-cycle | 87.907988 mol |
| Net H2 product per bed-cycle | 1158.055601 mol |
| H2 recovery | 0.864500260 |
| Plant-average H2 product flow | 25.549873 mol/s |
| Two-bed cycle period from throughput accounting | 90.650596 s |

The sealed mixing pressure rises slightly for this scenario: gas quantity
is fixed, but the common temperature is weighted by the gas, carbon and
adsorbate heat capacities, not solely by gas volume. Neither an increase
nor a decrease is imposed by the component.

## Validation and remaining work

The isothermal variant determines the bed heating/cooling duties instead
of setting them to zero. It anchors temperature at Tfeed during
repressurisation, avoiding a redundant temperature equality around the
closed cycle. At 299 K it matches the earlier isothermal model's feed and
gross product, with exactly one final void inventory more purge H2 due to
the thesis closure. Its recovery is 0.879103502. Setting only Hads to zero
does **not** make the adiabatic cycle isothermal: filling and expansion
still exchange flow/internal energy.

The ten tests cover:

- all operation and external-plant CH4, H2 and energy balances;
- a separate standard-library cycle-marching reference, with scalar roots
  for each operation (53 cycles to converge at the default point);
- dimensions, caloric-reference invariance and geometric scaling;
- pressure, temperature, adsorption-heat and purge-exhaust weighting changes;
- the isothermal mass-cycle comparison and zero-adsorption-heat case;
- integrated blowdown exhaust enthalpy, mixing pressure, and IPOPT agreement.

Energy equations within nonlinear blocks use temperature-scaled residuals
to avoid stagnation on tiny absolute joule residuals. The tests also check
balances in joules; solver tolerances are not loosened to obtain convergence.
The test script is included in the existing PSA CI step.

This remains a **zero-equalisation, fixed-design algebraic cycle**. Pure
H2 product is assumed, not predicted. The effective purge-exhaust
temperature is not a resolved profile; thermal pressure equalisation,
kinetic durations, feasible scheduling, economic optimisation and dynamic
purity validation are not implemented here. For the original **Part I
reproduction goal**, the next work is the [dedicated journal benchmark](psa_part1.md),
journal-specific operation closures and economics. The thesis's isothermal
dynamic model in section 7.4 is a later, separate validation task before
claiming that these algebraic designs satisfy the product-purity constraint.

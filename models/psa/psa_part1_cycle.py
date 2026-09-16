#!/usr/bin/env python3
"""Solve/report the closed zero-PE Part I equation reconstruction.

This is an equation audit, not an energy-conserving or validated design.
"""
import argparse
import importlib.util
import json
import math
from pathlib import Path

PSI = 6894.757293168


def _validate_options(pressure_basis, source_pressure, K_multiplier, adsorption_heat, fit):
    if pressure_basis not in ('reconciled', 'valve'):
        raise ValueError('Pressure basis must be reconciled or valve')
    if fit not in ('none', 'purge', 'adsorption-purge'):
        raise ValueError('Fit must be none, purge or adsorption-purge')
    if not all(math.isfinite(x) and x > 0 for x in (source_pressure, K_multiplier)):
        raise ValueError('Source pressure and K multiplier must be finite and positive')
    if not math.isfinite(adsorption_heat) or adsorption_heat < 0:
        raise ValueError('Adsorption heat must be finite and nonnegative')


def _physical_warnings(m, source_pressure, fitted):
    def v(x): return x.getRealValue()
    warnings = []
    if fitted:
        warnings.append('Inverse diagnostic: fitted K/heat are conditional calibration values, NOT recovered historical inputs or validation.')
    if v(m.bed.H) < 0:
        warnings.append('Inferred adsorption heat is NEGATIVE: incompatible with the assumed exothermic heat-release magnitude. This is an algebraic diagnostic, not a physical fit.')
    if v(m.bd.Pvalve) < 101325:
        warnings.append('Blowdown endpoint is below atmospheric pressure: a low-pressure sink/vacuum would be required; none is modelled.')
    if source_pressure < v(m.fr.Pvalve):
        warnings.append('Feed valve endpoint exceeds reservoir pressure: additional pressurisation/work is not modelled.')
    if abs(v(m.energy_defect)) > .1:
        warnings.append('The printed-equation cycle has a nonzero energy-accounting defect; it is not a conservative thermal design.')
    return warnings


def solve(pressure_basis='reconciled', source_pressure=300*PSI, K_multiplier=1, adsorption_heat=20920,
          fit='none'):
    import ascpy
    _validate_options(pressure_basis, source_pressure, K_multiplier, adsorption_heat, fit)
    lib = ascpy.Library()
    name = 'psa_part1_cycle' if pressure_basis == 'reconciled' else 'psa_part1_cycle_valve'
    try:
        typ = lib.findType(name)
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
        typ = lib.findType(name)
    sim = typ.getSimulation('part1_closed_report', True)
    m = sim.getModel()
    def run():
        sim.solve(ascpy.Solver('QRSlv'), ascpy.SolverReporter())
        if not sim.getStatus().isConverged():
            raise RuntimeError('Journal-cycle equations did not converge')
    sim.checkDimensions()
    run()
    # Explicit continuation from the baseline for changed assumptions;
    # no reference results, published amounts or fitted values initialise it.
    if (source_pressure, K_multiplier, adsorption_heat) != (300*PSI, 1, 20920):
        m.Psource.setRealValueWithUnits(source_pressure, 'Pa')
        m.K_multiplier.setRealValue(K_multiplier)
        m.bed.H.setRealValueWithUnits(adsorption_heat, 'J/mol')
        run()
    fitted = []
    if fit != 'none':
        method = 'fit_purge' if fit == 'purge' else 'fit_adsorption_purge'
        sim.run(next(x for x in typ.getMethods() if str(x.getName()) == method))
        run()
        fitted = ['purge_H2'] if fit == 'purge' else ['ads_feed', 'purge_H2']
    def v(x): return x.getRealValue()
    states = {key: {field: v(getattr(m.state[key], field)) for field in ('T','P','gA','gH','sA','U')}
              for key in ('pressurised','mixed','blown_down','regenerated')}
    defects = dict(adsorption=v(m.ads_defect), blowdown=v(m.bd.energy_defect),
                   purge=v(m.purge_defect), repressurisation=v(m.fr.energy_defect))
    amounts = dict(ads_feed=v(m.ads.feed), fr_feed=v(m.fr.feed), gross_H2=v(m.ads.product),
                   purge_H2=v(m.des.hydrogen_in))
    published = {k: v(m.data.amount[k]['pe0']) for k in amounts}
    # Load by path as this driver is also imported from outside models/.
    spec = importlib.util.spec_from_file_location('psa_part1_bounds', Path(__file__).with_name('psa_part1_bounds.py'))
    bounds = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bounds)
    heat_check = bounds.zero_pe_heat_check(d=v(m.data.size['d']['pe0']), L=v(m.data.size['L']['pe0']),
                                          P=v(m.data.P['P9']['pe0']), ads_feed=published['ads_feed'],
                                          fr_feed=published['fr_feed'], gross_H2=published['gross_H2'])
    half_digit = dict(ads_feed=.5, fr_feed=.05, gross_H2=.5, purge_H2=.05)
    comparison = {k: dict(role='fitted target' if k in fitted else 'independent check',
                          error_mol=amounts[k]-published[k],
                          printed_half_digit_mol=half_digit[k],
                          within_printed_rounding=abs(amounts[k]-published[k]) <= half_digit[k])
                  for k in amounts}
    warnings = _physical_warnings(m, source_pressure, fitted)
    return dict(scope='Closed Part I equation audit, not a validated optimum', pressure_basis=pressure_basis,
                assumptions=dict(source_pressure_Pa=source_pressure, K_multiplier=v(m.K_multiplier),
                                 adsorption_heat_J_mol=v(m.bed.H), K='Henry-slope hypothesis',
                                 mixing='sealed, caloric, frozen solid', exhaust_T='purge final bed temperature'),
                calibration=dict(mode=fit, fitted_amounts=fitted,
                                 fitted_parameters=([] if not fitted else ['K_multiplier']
                                                    + (['adsorption_heat_J_mol'] if fit == 'adsorption-purge' else []))),
                amount_comparison=comparison,
                source_adsorption_heat_check=heat_check,
                states_SI=states, amounts_mol=amounts, published_amounts_mol=published,
                recovery=v(m.recovery), published_recovery=v(m.data.recovery['pe0']),
                published_amount_recovery=(published['gross_H2']-published['purge_H2'])/
                                         (.95*(published['ads_feed']+published['fr_feed'])),
                net_product_mol=v(m.product), period_s=v(m.period),
                blowdown_valve_Pa=v(m.bd.Pvalve), repressurisation_valve_Pa=v(m.fr.Pvalve),
                blowdown_gas_endpoint_K=v(m.bd.Tgas), initial_H2_compression_endpoint_K=v(m.fr.TB),
                feed_gas_endpoint_K=v(m.fr.TF),
                component_residuals_mol=dict(CH4=v(m.y)*v(m.feed)-v(m.wasteA),
                                             H2=(1-v(m.y))*v(m.feed)-v(m.product)-v(m.wasteH)),
                operation_energy_defects_J=defects, plant_energy_defect_J=v(m.energy_defect),
                energy_defect_sum_error_J=v(m.energy_defect)-sum(defects.values()),
                warnings=warnings)


def report(r):
    print(f"PART I ZERO-PE CYCLE — table pressure basis: {r['pressure_basis']}")
    print('Closed equation audit; NOT a reproduced optimum or validated physical design.')
    if r['calibration']['mode'] != 'none':
        print('INVERSE DIAGNOSTIC: fitting '+', '.join(r['calibration']['fitted_amounts'])
              +'; other amounts are held-out checks, not targets.')
    print(f"Assumptions: source pressure {r['assumptions']['source_pressure_Pa']/1e5:.5f} bar (unsourced), "
          f"Hads={r['assumptions']['adsorption_heat_J_mol']:g} J/mol, "
          f"K multiplier={r['assumptions']['K_multiplier']:g} on Henry-slope hypothesis.")
    print('\nQuantity (mol/bed-cycle)    Calculated      Part I Table 4')
    for k,label in (('ads_feed','Adsorption feed'),('fr_feed','Repressurisation feed'),
                    ('gross_H2','Gross H2 product'),('purge_H2','H2 purge supply')):
        check = r['amount_comparison'][k]
        print(f"{label:25s} {r['amounts_mol'][k]:10.3f}      {r['published_amounts_mol'][k]:10.3f}"
              f"  {check['role']}; {'within' if check['within_printed_rounding'] else 'outside'} printed rounding")
    print(f"Recovery: {r['recovery']:.6f}; printed {r['published_recovery']:.2f}; "
          f"from published amounts {r['published_amount_recovery']:.6f}.")
    heat_check = r['source_adsorption_heat_check']
    low, high = heat_check['rounding_enclosure']['heat_J_mol']
    print(f'\nSeparate source-data check (P9 interpreted as adsorption pressure): '
          f'necessary heat enclosure [{low/1000:.3f}, {high/1000:.3f}] kJ/mol.')
    print('Includes displayed rounding of flows, geometry and pressure; other parameters fixed. '
          'Not a feasible heat range for the full cycle.')
    print('Cen–Yang 20.920 kJ/mol is '+('excluded.' if heat_check['reference_heat_excluded'] else 'not excluded.'))
    print('\nBoundary                 T (K)       P (bar abs)     Solid CH4 (mol)')
    for key,s in r['states_SI'].items():
        print(f"{key:22s} {s['T']:9.3f}      {s['P']/1e5:10.5f}       {s['sA']:10.3f}")
    print(f"Valve pressures before equilibration: BD={r['blowdown_valve_Pa']/1e5:.5f}, "
          f"FR={r['repressurisation_valve_Pa']/1e5:.5f} bar abs.")
    print(f"Gas-only endpoint temperatures: BD={r['blowdown_gas_endpoint_K']:.2f}, "
          f"initial H2={r['initial_H2_compression_endpoint_K']:.2f}, "
          f"feed={r['feed_gas_endpoint_K']:.2f} K.")
    print('\nEnergy-accounting defects (kJ/bed-cycle; NOT compensating heat duties):')
    for key,value in r['operation_energy_defects_J'].items(): print(f'  {key:20s} {value/1000:10.3f}')
    print(f"  {'whole cycle':20s} {r['plant_energy_defect_J']/1000:10.3f}")
    print('Sign convention: change in stored energy + outflow enthalpy − inflow enthalpy.')
    for warning in r['warnings']: print('WARNING: '+warning)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pressure-basis', choices=('reconciled','valve'), default='reconciled')
    parser.add_argument('--source-pressure-bar', type=float, default=300*PSI/1e5,
                        help='Unsourced feed reservoir pressure (default: 300 international psi in bar)')
    parser.add_argument('--k-multiplier', type=float, default=1)
    parser.add_argument('--adsorption-heat', type=float, default=20920, metavar='J_PER_MOL')
    parser.add_argument('--fit', choices=('none','purge','adsorption-purge'), default='none',
                        help='Explicit inverse diagnostic: fit K, or K and heat, to selected Table 4 amounts; NOT validation')
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args(argv)
    try:
        r=solve(args.pressure_basis,args.source_pressure_bar*1e5,args.k_multiplier,args.adsorption_heat,args.fit)
    except (ValueError,RuntimeError) as exc:
        parser.exit(1,f'Part I cycle: {exc}\n')
    if args.json: print(json.dumps(r,indent=2,allow_nan=False))
    else: report(r)


if __name__=='__main__': main()
